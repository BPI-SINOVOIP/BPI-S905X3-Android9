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

package com.android.tv.ui;

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ValueAnimator;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.media.tv.TvContentRating;
import android.media.tv.TvInputInfo;
import android.media.tv.TvContract;
import android.media.tv.TvTrackInfo;
import android.net.Uri;
import android.os.Handler;
import android.support.annotation.Nullable;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.text.style.TextAppearanceSpan;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityManager.AccessibilityStateChangeListener;
import android.view.animation.AnimationUtils;
import android.view.animation.Interpolator;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.graphics.drawable.Drawable;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.TvSingletons;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.data.Program;
import com.android.tv.data.StreamInfo;
import com.android.tv.data.api.Channel;
import com.android.tv.dvr.DvrManager;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.parental.ContentRatingsManager;
import com.android.tv.ui.TvTransitionManager.TransitionLayout;
import com.android.tv.ui.hideable.AutoHideScheduler;
import com.android.tv.util.Utils;
import com.android.tv.util.images.ImageCache;
import com.android.tv.util.images.ImageLoader;
import com.android.tv.util.images.ImageLoader.ImageLoaderCallback;
import com.android.tv.util.images.ImageLoader.LoadTvInputLogoTask;
import com.android.tv.common.util.SystemProperties;

import com.android.tv.util.TvSettings;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;

/** A view to render channel banner. */
public class ChannelBannerView extends FrameLayout
        implements TransitionLayout, AccessibilityStateChangeListener {
    private static final String TAG = "ChannelBannerView";
    private static final boolean DEBUG = false || SystemProperties.USE_DEBUG_CHANNEL_UPDATE.getValue();

    /** Show all information at the channel banner. */
    public static final int LOCK_NONE = 0;

    /**
     * Lock program details at the channel banner. This is used when a content is locked so we don't
     * want to show program details including program description text and poster art.
     */
    public static final int LOCK_PROGRAM_DETAIL = 1;

    /**
     * Lock channel information at the channel banner. This is used when a channel is locked so we
     * only want to show input information.
     */
    public static final int LOCK_CHANNEL_INFO = 2;

    private static final int DISPLAYED_CONTENT_RATINGS_COUNT = 3;

    private static final String EMPTY_STRING = "";
    private static final String ATVRESOLUTION_NTSC = "480i";
    private static final String ATVRESOLUTION_PAL = "576i";
    private static final String ATVRESOLUTION_SECAM = "576i";

    private static final String ATV_SYS_COLOR_FLAG = "video";
    private static final String ATV_SYS_SOUND_FLAG = "audio";

    private Program mNoProgram;
    private Program mLockedChannelProgram;
    private static String sClosedCaptionMark;

    private final MainActivity mMainActivity;
    private final Resources mResources;
    private View mChannelView;

    private TextView mChannelNumberTextView;
    private ImageView mChannelLogoImageView;
    private TextView mProgramTextView;
    private ImageView mTvInputLogoImageView;
    private TextView mChannelNameTextView;
    private TextView mProgramTimeTextView;
    private ProgressBar mRemainingTimeView;
    private TextView mRecordingIndicatorView;
    private TextView mClosedCaptionTextView;
    private TextView mAspectRatioTextView;
    private TextView mResolutionTextView;
    private TextView mAtvColorSysTextView;
    private TextView mAtvSoundSysTextView;
    private TextView mAudioChannelTextView;
    private TextView[] mContentRatingsTextViews = new TextView[DISPLAYED_CONTENT_RATINGS_COUNT];
    private TextView mVideoFormatTextView;
    private TextView mAudioFormatTextView;
    private TextView mTimeTextView;
    private TextView mProgramDescriptionTextView;
    private String mProgramDescriptionText;
    private View mAnchorView;
    private Channel mCurrentChannel;
    private boolean mCurrentChannelLogoExists;
    private Program mLastUpdatedProgram;
    private final AutoHideScheduler mAutoHideScheduler;
    private final DvrManager mDvrManager;
    private ContentRatingsManager mContentRatingsManager;
    private TvContentRating mBlockingContentRating;

    private int mLockType;
    private boolean mUpdateOnTune;

    private Animator mResizeAnimator;
    private int mCurrentHeight;
    private boolean mProgramInfoUpdatePendingByResizing;

    private final Animator mProgramDescriptionFadeInAnimator;
    private final Animator mProgramDescriptionFadeOutAnimator;

    private final long mShowDurationMillis;
    private final int mChannelLogoImageViewWidth;
    private final int mChannelLogoImageViewHeight;
    private final int mChannelLogoImageViewMarginStart;
    private final int mProgramDescriptionTextViewWidth;
    private final int mChannelBannerTextColor;
    private final int mChannelBannerDimTextColor;
    private final int mResizeAnimDuration;
    private final int mRecordingIconPadding;
    private final Interpolator mResizeInterpolator;

    private final AnimatorListenerAdapter mResizeAnimatorListener =
            new AnimatorListenerAdapter() {
                @Override
                public void onAnimationStart(Animator animator) {
                    mProgramInfoUpdatePendingByResizing = false;
                }

                @Override
                public void onAnimationEnd(Animator animator) {
                    mProgramDescriptionTextView.setAlpha(1f);
                    mResizeAnimator = null;
                    if (mProgramInfoUpdatePendingByResizing) {
                        mProgramInfoUpdatePendingByResizing = false;
                        updateProgramInfo(mLastUpdatedProgram);
                    }
                }
            };

    public ChannelBannerView(Context context) {
        this(context, null);
    }

    public ChannelBannerView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ChannelBannerView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mResources = getResources();
        mMainActivity = (MainActivity) context;

        mShowDurationMillis = mResources.getInteger(R.integer.channel_banner_show_duration);
        mChannelLogoImageViewWidth =
                mResources.getDimensionPixelSize(R.dimen.channel_banner_channel_logo_width);
        mChannelLogoImageViewHeight =
                mResources.getDimensionPixelSize(R.dimen.channel_banner_channel_logo_height);
        mChannelLogoImageViewMarginStart =
                mResources.getDimensionPixelSize(R.dimen.channel_banner_channel_logo_margin_start);
        mProgramDescriptionTextViewWidth =
                mResources.getDimensionPixelSize(R.dimen.channel_banner_program_description_width);
        mChannelBannerTextColor = mResources.getColor(R.color.channel_banner_text_color, null);
        mChannelBannerDimTextColor =
                mResources.getColor(R.color.channel_banner_dim_text_color, null);
        mResizeAnimDuration = mResources.getInteger(R.integer.channel_banner_fast_anim_duration);
        mRecordingIconPadding =
                mResources.getDimensionPixelOffset(R.dimen.channel_banner_recording_icon_padding);

        mResizeInterpolator =
                AnimationUtils.loadInterpolator(context, android.R.interpolator.linear_out_slow_in);

        mProgramDescriptionFadeInAnimator =
                AnimatorInflater.loadAnimator(
                        mMainActivity, R.animator.channel_banner_program_description_fade_in);
        mProgramDescriptionFadeOutAnimator =
                AnimatorInflater.loadAnimator(
                        mMainActivity, R.animator.channel_banner_program_description_fade_out);

        if (CommonFeatures.DVR.isEnabled(mMainActivity)) {
            mDvrManager = TvSingletons.getSingletons(mMainActivity).getDvrManager();
        } else {
            mDvrManager = null;
        }
        mContentRatingsManager =
                TvSingletons.getSingletons(getContext())
                        .getTvInputManagerHelper()
                        .getContentRatingsManager();

        mNoProgram =
                new Program.Builder()
                        .setTitle(context.getString(R.string.channel_banner_no_title))
                        .setDescription(EMPTY_STRING)
                        .build();
        mLockedChannelProgram =
                new Program.Builder()
                        .setTitle(context.getString(R.string.channel_banner_locked_channel_title))
                        .setDescription(EMPTY_STRING)
                        .build();
        if (sClosedCaptionMark == null) {
            sClosedCaptionMark = context.getString(R.string.closed_caption);
        }
        mAutoHideScheduler = new AutoHideScheduler(context, this::hide);
        context.getSystemService(AccessibilityManager.class)
                .addAccessibilityStateChangeListener(mAutoHideScheduler);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mChannelView = findViewById(R.id.channel_banner_view);

        mChannelNumberTextView = (TextView) findViewById(R.id.channel_number);
        mChannelLogoImageView = (ImageView) findViewById(R.id.channel_logo);
        mProgramTextView = (TextView) findViewById(R.id.program_text);
        mTvInputLogoImageView = (ImageView) findViewById(R.id.tvinput_logo);
        mChannelNameTextView = (TextView) findViewById(R.id.channel_name);
        mProgramTimeTextView = (TextView) findViewById(R.id.program_time_text);
        mRemainingTimeView = (ProgressBar) findViewById(R.id.remaining_time);
        mRecordingIndicatorView = (TextView) findViewById(R.id.recording_indicator);
        mClosedCaptionTextView = (TextView) findViewById(R.id.closed_caption);
        mAspectRatioTextView = (TextView) findViewById(R.id.aspect_ratio);
        mResolutionTextView = (TextView) findViewById(R.id.resolution);
        mAtvColorSysTextView = (TextView) findViewById(R.id.atv_color_sytem);
        mAtvSoundSysTextView = (TextView) findViewById(R.id.atv_sound_sytem);
        mAudioChannelTextView = (TextView) findViewById(R.id.audio_channel);
        mContentRatingsTextViews[0] = (TextView) findViewById(R.id.content_ratings_0);
        mContentRatingsTextViews[1] = (TextView) findViewById(R.id.content_ratings_1);
        mContentRatingsTextViews[2] = (TextView) findViewById(R.id.content_ratings_2);
        mVideoFormatTextView = (TextView) findViewById(R.id.video_format);
        mAudioFormatTextView = (TextView) findViewById(R.id.audio_format);
        mTimeTextView = (TextView) findViewById(R.id.time);
        mProgramDescriptionTextView = (TextView) findViewById(R.id.program_description);
        mAnchorView = findViewById(R.id.anchor);

        mProgramDescriptionFadeInAnimator.setTarget(mProgramDescriptionTextView);
        mProgramDescriptionFadeOutAnimator.setTarget(mProgramDescriptionTextView);
        mProgramDescriptionFadeOutAnimator.addListener(
                new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animator) {
                        mProgramDescriptionTextView.setText(mProgramDescriptionText);
                    }
                });
    }

    @Override
    public void onEnterAction(boolean fromEmptyScene) {
        resetAnimationEffects();
        if (fromEmptyScene) {
            ViewUtils.setTransitionAlpha(mChannelView, 1f);
        }
        mAutoHideScheduler.schedule(mShowDurationMillis);
        updateViews(mMainActivity.getTvView());
    }

    @Override
    public void onExitAction() {
        mCurrentHeight = 0;
        mAutoHideScheduler.cancel();
    }

    private void resetAnimationEffects() {
        setAlpha(1f);
        setScaleX(1f);
        setScaleY(1f);
        setTranslationX(0);
        setTranslationY(0);
    }

    /**
     * Set new lock type.
     *
     * @param lockType Any of LOCK_NONE, LOCK_PROGRAM_DETAIL, or LOCK_CHANNEL_INFO.
     * @return the previous lock type of the channel banner.
     * @throws IllegalArgumentException if lockType is invalid.
     */
    public int setLockType(int lockType) {
        if (lockType != LOCK_NONE
                && lockType != LOCK_CHANNEL_INFO
                && lockType != LOCK_PROGRAM_DETAIL) {
            throw new IllegalArgumentException("No such lock type " + lockType);
        }
        int previousLockType = mLockType;
        mLockType = lockType;
        return previousLockType;
    }

    /**
     * Sets the content rating that blocks the current watched channel for displaying it in the
     * channel banner.
     */
    public void setBlockingContentRating(TvContentRating rating) {
        mBlockingContentRating = rating;
        updateProgramRatings(mMainActivity.getCurrentProgram());
    }

    /**
     * Update channel banner view.
     *
     * @param updateOnTune {@false} denotes the channel banner is updated due to other reasons than
     *     tuning. The channel info will not be updated in this case.
     */
    public void updateViews(boolean updateOnTune) {
        resetAnimationEffects();
        mChannelView.setVisibility(VISIBLE);
        mUpdateOnTune = updateOnTune;
        if (mUpdateOnTune) {
            if (isShown()) {
                mAutoHideScheduler.schedule(mShowDurationMillis);
            }
            mBlockingContentRating = null;
            mCurrentChannel = mMainActivity.getCurrentChannel();
            Log.d(TAG, "updateViews updateOnTune " + mCurrentChannel);
            mCurrentChannelLogoExists =
                    mCurrentChannel != null && mCurrentChannel.channelLogoExists();
            updateStreamInfo(null);
            updateChannelInfo();
        } else {
            updateProgramInfo(mMainActivity.getCurrentProgram());
        }
        mUpdateOnTune = false;
    }

    private void hide() {
        mCurrentHeight = 0;
        mMainActivity
                .getOverlayManager()
                .hideOverlays(
                        TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_DIALOG
                                | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_SIDE_PANELS
                                | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_PROGRAM_GUIDE
                                | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_MENU
                                | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_FRAGMENT);
    }

    //force refresh when info is pressed
    public void updateViews(StreamInfo info) {
         resetAnimationEffects();
         mChannelView.setVisibility(VISIBLE);
         mCurrentChannel = mMainActivity.getCurrentChannel();
         updateStreamInfo(info);
         updateChannelInfo();
         updateProgramInfo(mMainActivity.getCurrentProgram());
         Log.d(TAG, "updateViews StreamInfo " + mCurrentChannel);
    }

    /**
     * Update channel banner view with stream info.
     *
     * @param info A StreamInfo that includes stream information.
     */
    public void updateStreamInfo(StreamInfo info) {
        // Update stream information in a channel.
        if (mLockType != LOCK_CHANNEL_INFO && info != null) {
            updateText(
                    mClosedCaptionTextView,
                    info.hasClosedCaption() ? /*sClosedCaptionMark*/
                    (info.getSubtitleLabel() == null ? sClosedCaptionMark : info.getSubtitleLabel()) : EMPTY_STRING);
            updateText(
                    mAudioChannelTextView,
                    Utils.getAudioChannelString(mMainActivity, info.getAudioChannelCount()));
            updateText(mAspectRatioTextView, Utils.getAspectRatioString(info.getVideoDisplayAspectRatio()));
            //parse reslution from videoformat
            String videoformat = mCurrentChannel != null ? mCurrentChannel.getVideoFormat() : "";
            String resolution = parseReslutionFromVideoFormat(videoformat);
            if (TextUtils.isEmpty(resolution)) {
                String piFormat = (!TextUtils.isEmpty(info.getVideoPiFormat())) ? info.getVideoPiFormat() : (info.getVideoHeight() > 0 ? (info.getVideoHeight() + " ") : "");
                if (!TextUtils.isEmpty(piFormat)) {
                    resolution = piFormat + " "   + Utils.getVideoDefinitionLevelString(
                        mMainActivity, info.getVideoDefinitionLevel());
                } else {
                    resolution = "";
                }
            }
            updateText(mResolutionTextView, resolution
                    /*Utils.getVideoDefinitionLevelString(
                            mMainActivity, info.getVideoDefinitionLevel())*/);
            if (mCurrentChannel != null && !mCurrentChannel.isPassthrough()) {
                String audiotext;
                if (mMainActivity.mQuickKeyInfo.isAtvSource()) {
                    updateText(mAtvColorSysTextView, getCurrentColorOrSound(ATV_SYS_COLOR_FLAG));
                    updateText(mAtvSoundSysTextView, getCurrentColorOrSound(ATV_SYS_SOUND_FLAG));
                    audiotext = mMainActivity.mQuickKeyInfo.getAtvAudioStreamOutmodestring();
                } else {
                    TvTrackInfo currentTrack = mMainActivity.getCurrentTrackInfo();
                    audiotext = currentTrack != null ? currentTrack.getLanguage() : "";
                }
                updateText(mAudioChannelTextView, audiotext
                    /*Utils.getAudioChannelString(mMainActivity, info.getAudioChannelCount())*/);
            }
            updateText(mVideoFormatTextView, mMainActivity.mQuickKeyInfo.getVideoFormat());
            if (mMainActivity.mQuickKeyInfo.isAudioFormatAC3()) {
                updateText(mAudioFormatTextView,  mMainActivity.getResources().getDrawable(R.drawable.certifi_dobly_white));
            } else {
                updateText(mAudioFormatTextView,  mMainActivity.mQuickKeyInfo.getAudioFormat(false, null));
            }
            updateText(mTimeTextView, mCurrentChannel != null ? mMainActivity.mQuickKeyInfo.getDtvTime(!mCurrentChannel.isAnalogChannel()) : null);
        } else {
            // Channel change has been requested. But, StreamInfo hasn't been updated yet.
            mClosedCaptionTextView.setVisibility(View.GONE);
            mAspectRatioTextView.setVisibility(View.GONE);
            mResolutionTextView.setVisibility(View.GONE);
            mAtvColorSysTextView.setVisibility(View.GONE);
            mAtvSoundSysTextView.setVisibility(View.GONE);
            mAudioChannelTextView.setVisibility(View.GONE);
            mTimeTextView.setVisibility(View.GONE);
            mVideoFormatTextView.setVisibility(View.GONE);
            mAudioFormatTextView.setVisibility(View.GONE);
        }
    }

    private String getCurrentColorOrSound(String flag) {
        String colorOrSoundType = "";
        TvDataBaseManager mTvDataBaseManager = new TvDataBaseManager(mMainActivity);
        Uri channelUri = TvContract.buildChannelUri(mCurrentChannel != null ? mCurrentChannel.getId() : -1);
        ChannelInfo channelInfo = mTvDataBaseManager.getChannelInfo(channelUri);
        if (channelInfo == null) {
            Log.e(TAG,"Can't get current channel info when get current Color or Sound!");
            return colorOrSoundType;
        }
        if (flag.equals(ATV_SYS_COLOR_FLAG)) {
            int videoStd = channelInfo.getVideoStd();
            switch (videoStd) {
                case TvControlManager.ATV_VIDEO_STD_PAL:
                    int vfmt = channelInfo.getVfmt();
                    if ((vfmt & 0x00ffffff) == TvControlManager.V4L2_STD_PAL_M) {
                        colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_video_pal_m);
                    } else if ((vfmt & 0x00ffffff) == TvControlManager.V4L2_STD_PAL_Nc) {
                        colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_video_pal_n);
                    } else if ((vfmt & 0x00ffffff) == TvControlManager.V4L2_STD_PAL_60) {
                        colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_video_pal_60);
                    } else {
                        colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_video_pal);
                    }
                    break;
                case TvControlManager.ATV_VIDEO_STD_NTSC:
                    colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_video_ntsc);
                    break;
                case TvControlManager.ATV_VIDEO_STD_SECAM:
                    colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_video_secam);
                    break;
            }
        } else if (flag.equals(ATV_SYS_SOUND_FLAG)) {
            int audioStd = channelInfo.getAudioStd();
            switch (audioStd) {
                case TvControlManager.ATV_AUDIO_STD_DK:
                    colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_audio_dk);
                    break;
                case TvControlManager.ATV_AUDIO_STD_I:
                    colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_audio_i);
                    break;
                case TvControlManager.ATV_AUDIO_STD_BG:
                    colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_audio_bg);
                    break;
                case TvControlManager.ATV_AUDIO_STD_M:
                    colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_audio_m);
                    break;
                case TvControlManager.ATV_AUDIO_STD_L:
                    colorOrSoundType = mMainActivity.getResources().getString(R.string.channel_audio_l);
                    break;
            }
        }
        return colorOrSoundType;
    }

    private void updateChannelInfo() {
        // Update static information for a channel.
        String displayNumber = EMPTY_STRING;
        String displayName = EMPTY_STRING;
        if (mCurrentChannel != null) {
            displayNumber = mCurrentChannel.getDisplayNumber();
            if (displayNumber == null) {
                displayNumber = EMPTY_STRING;
            }
            displayName = mCurrentChannel.getDisplayName();
            if (displayName == null) {
                displayName = EMPTY_STRING;
            }
        }

        if (displayNumber.isEmpty()) {
            mChannelNumberTextView.setVisibility(GONE);
        } else {
            mChannelNumberTextView.setVisibility(VISIBLE);
        }
        if (displayNumber.length() <= 3) {
            updateTextView(
                    mChannelNumberTextView,
                    R.dimen.channel_banner_channel_number_large_text_size,
                    R.dimen.channel_banner_channel_number_large_margin_top);
        } else if (displayNumber.length() <= 4) {
            updateTextView(
                    mChannelNumberTextView,
                    R.dimen.channel_banner_channel_number_medium_text_size,
                    R.dimen.channel_banner_channel_number_medium_margin_top);
        } else {
            updateTextView(
                    mChannelNumberTextView,
                    R.dimen.channel_banner_channel_number_small_text_size,
                    R.dimen.channel_banner_channel_number_small_margin_top);
        }
        mChannelNumberTextView.setText(displayNumber);
        mChannelNameTextView.setText(displayName);
        TvInputInfo info =
                mMainActivity.getTvInputManagerHelper().getTvInputInfo(getCurrentInputId());
        if (info == null
                || !ImageLoader.loadBitmap(
                        createTvInputLogoLoaderCallback(info, this),
                        new LoadTvInputLogoTask(getContext(), ImageCache.getInstance(), info))) {
            mTvInputLogoImageView.setVisibility(View.GONE);
            mTvInputLogoImageView.setImageDrawable(null);
        }
        mChannelLogoImageView.setImageBitmap(null);
        mChannelLogoImageView.setVisibility(View.GONE);
        if (mCurrentChannel != null && mCurrentChannelLogoExists) {
            mCurrentChannel.loadBitmap(
                    getContext(),
                    Channel.LOAD_IMAGE_TYPE_CHANNEL_LOGO,
                    mChannelLogoImageViewWidth,
                    mChannelLogoImageViewHeight,
                    createChannelLogoCallback(this, mCurrentChannel));
        }
    }

    private String getCurrentInputId() {
        return mCurrentChannel != null ? mCurrentChannel.getInputId() : null;
    }

    private void updateTvInputLogo(Bitmap bitmap) {
        mTvInputLogoImageView.setVisibility(View.VISIBLE);
        mTvInputLogoImageView.setImageBitmap(bitmap);
    }

    private static ImageLoaderCallback<ChannelBannerView> createTvInputLogoLoaderCallback(
            final TvInputInfo info, ChannelBannerView channelBannerView) {
        return new ImageLoaderCallback<ChannelBannerView>(channelBannerView) {
            @Override
            public void onBitmapLoaded(ChannelBannerView channelBannerView, Bitmap bitmap) {
                if (bitmap != null
                        && channelBannerView.mCurrentChannel != null
                        && info.getId().equals(channelBannerView.mCurrentChannel.getInputId())) {
                    channelBannerView.updateTvInputLogo(bitmap);
                }
            }
        };
    }

    private void updateText(TextView view, String text) {
        if (TextUtils.isEmpty(text)) {
            view.setVisibility(View.GONE);
        } else {
            view.setBackgroundColor(mMainActivity.getResources().getColor(R.color.channel_banner_program_track_meta_back));
            view.setText(text);
            view.setVisibility(View.VISIBLE);
        }
    }

    private void updateText(TextView view, Drawable draw) {
        if (draw == null) {
            view.setBackground(null);
            view.setVisibility(View.GONE);
        } else {
            view.setBackground(draw);
            view.setText("");
            view.setVisibility(View.VISIBLE);
        }
    }

    private void updateTextView(TextView textView, int sizeRes, int marginTopRes) {
        float textSize = mResources.getDimension(sizeRes);
        if (textView.getTextSize() != textSize) {
            textView.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
        }
        updateTopMargin(textView, marginTopRes);
    }

    private void updateTopMargin(View view, int marginTopRes) {
        RelativeLayout.LayoutParams lp = (RelativeLayout.LayoutParams) view.getLayoutParams();
        int topMargin = (int) mResources.getDimension(marginTopRes);
        if (lp.topMargin != topMargin) {
            lp.topMargin = topMargin;
            view.setLayoutParams(lp);
        }
    }

    private static ImageLoaderCallback<ChannelBannerView> createChannelLogoCallback(
            ChannelBannerView channelBannerView, final Channel channel) {
        return new ImageLoaderCallback<ChannelBannerView>(channelBannerView) {
            @Override
            public void onBitmapLoaded(ChannelBannerView view, @Nullable Bitmap logo) {
                if (channel.equals(view.mCurrentChannel)) {
                    // The logo is obsolete.
                    return;
                }
                view.updateLogo(logo);
            }
        };
    }

    private void updateLogo(@Nullable Bitmap logo) {
        if (logo == null) {
            // Need to update the text size of the program text view depending on the channel logo.
            updateProgramTextView(mLastUpdatedProgram);
            return;
        }

        mChannelLogoImageView.setImageBitmap(logo);
        mChannelLogoImageView.setVisibility(View.VISIBLE);
        updateProgramTextView(mLastUpdatedProgram);

        if (mResizeAnimator == null) {
            String description = mProgramDescriptionTextView.getText().toString();
            boolean programDescriptionNeedFadeAnimation =
                    !description.equals(mProgramDescriptionText) && !mUpdateOnTune;
            updateBannerHeight(programDescriptionNeedFadeAnimation);
        } else {
            mProgramInfoUpdatePendingByResizing = true;
        }
    }

    private void updateProgramInfo(Program program) {
        if (mLockType == LOCK_CHANNEL_INFO) {
            program = mLockedChannelProgram;
        } else if (program == null || !program.isValid() || TextUtils.isEmpty(program.getTitle())) {
            program = mNoProgram;
        }

        if (mLastUpdatedProgram == null
                || !TextUtils.equals(program.getTitle(), mLastUpdatedProgram.getTitle())
                || !TextUtils.equals(
                        program.getEpisodeDisplayTitle(getContext()),
                        mLastUpdatedProgram.getEpisodeDisplayTitle(getContext()))) {
            updateProgramTextView(program);
        }
        updateProgramTimeInfo(program);
        updateRecordingStatus(program);
        updateProgramRatings(program);

        // When the program is changed, but the previous resize animation has not ended yet,
        // cancel the animation.
        boolean isProgramChanged = !program.equals(mLastUpdatedProgram);
        if (mResizeAnimator != null && isProgramChanged) {
            setLastUpdatedProgram(program);
            mProgramInfoUpdatePendingByResizing = true;
            mResizeAnimator.cancel();
        } else if (mResizeAnimator == null) {
            if (mLockType != LOCK_NONE || TextUtils.isEmpty(program.getDescription())) {
                mProgramDescriptionTextView.setVisibility(GONE);
                mProgramDescriptionText = "";
            } else {
                mProgramDescriptionTextView.setVisibility(VISIBLE);
                mProgramDescriptionText = program.getDescription();
            }
            String description = mProgramDescriptionTextView.getText().toString();
            boolean programDescriptionNeedFadeAnimation =
                    (isProgramChanged || !description.equals(mProgramDescriptionText))
                            && !mUpdateOnTune;
            updateBannerHeight(programDescriptionNeedFadeAnimation);
        } else {
            mProgramInfoUpdatePendingByResizing = true;
        }
        setLastUpdatedProgram(program);
    }

    private void updateProgramTextView(Program program) {
        if (program == null) {
            return;
        }
        updateProgramTextView(
                program.equals(mLockedChannelProgram),
                program.getTitle(),
                program.getEpisodeDisplayTitle(getContext()));
    }

    private void updateProgramTextView(boolean dimText, String title, String episodeDisplayTitle) {
        mProgramTextView.setVisibility(View.VISIBLE);
        if (dimText) {
            mProgramTextView.setTextColor(mChannelBannerDimTextColor);
        } else {
            mProgramTextView.setTextColor(mChannelBannerTextColor);
        }
        updateTextView(
                mProgramTextView,
                R.dimen.channel_banner_program_large_text_size,
                R.dimen.channel_banner_program_large_margin_top);
        if (TextUtils.isEmpty(episodeDisplayTitle)) {
            mProgramTextView.setText(title);
        } else {
            String fullTitle = title + "  " + episodeDisplayTitle;

            SpannableString text = new SpannableString(fullTitle);
            text.setSpan(
                    new TextAppearanceSpan(
                            getContext(), R.style.text_appearance_channel_banner_episode_title),
                    fullTitle.length() - episodeDisplayTitle.length(),
                    fullTitle.length(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            mProgramTextView.setText(text);
        }
        int width =
                mProgramDescriptionTextViewWidth
                        + (mCurrentChannelLogoExists
                                ? 0
                                : mChannelLogoImageViewWidth + mChannelLogoImageViewMarginStart);
        ViewGroup.LayoutParams lp = mProgramTextView.getLayoutParams();
        lp.width = width;
        mProgramTextView.setLayoutParams(lp);
        mProgramTextView.measure(
                MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));

        boolean oneline = (mProgramTextView.getLineCount() == 1);
        if (!oneline) {
            updateTextView(
                    mProgramTextView,
                    R.dimen.channel_banner_program_medium_text_size,
                    R.dimen.channel_banner_program_medium_margin_top);
            mProgramTextView.measure(
                    MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
            oneline = (mProgramTextView.getLineCount() == 1);
        }
        updateTopMargin(
                mAnchorView,
                oneline
                        ? R.dimen.channel_banner_anchor_one_line_y
                        : R.dimen.channel_banner_anchor_two_line_y);
    }

    private void updateProgramRatings(Program program) {
        if (mLockType == LOCK_CHANNEL_INFO) {
            for (int i = 0; i < DISPLAYED_CONTENT_RATINGS_COUNT; i++) {
                mContentRatingsTextViews[i].setVisibility(View.GONE);
            }
        } else if (mBlockingContentRating != null) {
            String displayNameForRating =
                    mContentRatingsManager.getDisplayNameForRating(mBlockingContentRating);
            if (!TextUtils.isEmpty(displayNameForRating)) {
                mContentRatingsTextViews[0].setText(displayNameForRating);
                mContentRatingsTextViews[0].setVisibility(View.VISIBLE);
            } else {
                mContentRatingsTextViews[0].setVisibility(View.GONE);
            }
            for (int i = 1; i < DISPLAYED_CONTENT_RATINGS_COUNT; i++) {
                mContentRatingsTextViews[i].setVisibility(View.GONE);
            }
        } else {
            TvContentRating[] ratings = mMainActivity.mQuickKeyInfo.getContentRatingsOfCurrentProgram();/*(program == null) ? null : program.getContentRatings();*/
            for (int i = 0; i < DISPLAYED_CONTENT_RATINGS_COUNT; i++) {
                if (ratings == null || ratings.length <= i) {
                    mContentRatingsTextViews[i].setVisibility(View.GONE);
                } else {
                    mContentRatingsTextViews[i].setText(
                            mContentRatingsManager.getDisplayNameForRating(ratings[i]));
                    mContentRatingsTextViews[i].setVisibility(View.VISIBLE);
                }
            }
            if (ratings == null || (ratings != null && ratings.length == 0)) {
                mContentRatingsTextViews[0].setText(ContentRatingsManager.RATING_NO);
                if (TextUtils.equals(DroidLogicTvUtils.getCurrentSignalType(getContext()), TvContract.Channels.TYPE_DTMB)) {
                    mContentRatingsTextViews[0].setVisibility(View.GONE);
                } else {
                    mContentRatingsTextViews[0].setVisibility(View.VISIBLE);
                }
            }
        }
    }

    private void updateProgramTimeInfo(Program program) {
        long durationMs = program.getDurationMillis();
        long startTimeMs = program.getStartTimeUtcMillis();
        long endTimeMs = program.getEndTimeUtcMillis();

        if (mLockType != LOCK_CHANNEL_INFO && durationMs > 0 && startTimeMs > 0) {
            mProgramTimeTextView.setVisibility(View.VISIBLE);
            mRemainingTimeView.setVisibility(View.VISIBLE);
            mProgramTimeTextView.setText(
                    Utils.getDurationString(getContext(), startTimeMs, endTimeMs, true));
        } else {
            mProgramTimeTextView.setVisibility(View.GONE);
            mRemainingTimeView.setVisibility(View.GONE);
        }
    }

    private int getProgressPercent(long currTime, long startTime, long endTime) {
        if (currTime <= startTime) {
            return 0;
        } else if (currTime >= endTime) {
            return 100;
        } else {
            return (int) (100 * (currTime - startTime) / (endTime - startTime));
        }
    }

    private void updateRecordingStatus(Program program) {
        Log.d(TAG, "updateRecordingStatus " + mDvrManager);
        if (mDvrManager == null) {
            updateProgressBarAndRecIcon(program, null);
            return;
        }
        if (DEBUG) {
            Log.d(TAG, "updateRecordingStatus program = " + program + ", mCurrentChannel = " + mCurrentChannel);
        }
        ScheduledRecording currentRecording =
                (mCurrentChannel == null)
                        ? null
                        : mDvrManager.getCurrentRecording(mCurrentChannel.getId());
        if (DEBUG) {
            Log.d(TAG, currentRecording == null ? "No Recording" : "Recording:" + currentRecording);
        }
        if (currentRecording != null && isCurrentProgram(currentRecording, program)) {
            updateProgressBarAndRecIcon(program, currentRecording);
        } else {
            updateProgressBarAndRecIcon(program, null);
        }
    }

    private void updateProgressBarAndRecIcon(
            Program program, @Nullable ScheduledRecording recording) {
        long programStartTime = program.getStartTimeUtcMillis();
        long programEndTime = program.getEndTimeUtcMillis();
        long currentPosition = mMainActivity.getCurrentPlayingPosition();
        updateRecordingIndicator(recording);
        if (recording != null) {
            // Recording now. Use recording-style progress bar.
            mRemainingTimeView.setProgress(
                    getProgressPercent(
                            recording.getStartTimeMs(), programStartTime, programEndTime));
            mRemainingTimeView.setSecondaryProgress(
                    getProgressPercent(currentPosition, programStartTime, programEndTime));
        } else {
            // No recording is going now. Recover progress bar.
            mRemainingTimeView.setProgress(
                    getProgressPercent(currentPosition, programStartTime, programEndTime));
            mRemainingTimeView.setSecondaryProgress(0);
        }
    }

    private void updateRecordingIndicator(@Nullable ScheduledRecording recording) {
        if (recording != null) {
            if (mRemainingTimeView.getVisibility() == View.GONE) {
                mRecordingIndicatorView.setText(
                        mMainActivity
                                .getResources()
                                .getString(
                                        R.string.dvr_recording_till_format,
                                        DateUtils.formatDateTime(
                                                mMainActivity,
                                                recording.getEndTimeMs(),
                                                DateUtils.FORMAT_SHOW_TIME)));
                mRecordingIndicatorView.setCompoundDrawablePadding(mRecordingIconPadding);
            } else {
                mRecordingIndicatorView.setText("");
                mRecordingIndicatorView.setCompoundDrawablePadding(0);
            }
            mRecordingIndicatorView.setVisibility(View.VISIBLE);
        } else {
            mRecordingIndicatorView.setVisibility(View.GONE);
        }
    }

    private boolean isCurrentProgram(ScheduledRecording recording, Program program) {
        long currentPosition = mMainActivity.getCurrentPlayingPosition();
        return (recording.getType() == ScheduledRecording.TYPE_PROGRAM
                        && recording.getProgramId() == program.getId())
                || (recording.getType() == ScheduledRecording.TYPE_TIMED
                        /*&& currentPosition >= recording.getStartTimeMs()
                        && currentPosition <= recording.getEndTimeMs()*/);
        //TYPE_TIMED no need to check current time as stream card may play with a loop
    }

    private void setLastUpdatedProgram(Program program) {
        mLastUpdatedProgram = program;
    }

    private void updateBannerHeight(boolean needProgramDescriptionFadeAnimation) {
        SoftPreconditions.checkState(mResizeAnimator == null);
        // Need to measure the layout height with the new description text.
        CharSequence oldDescription = mProgramDescriptionTextView.getText();
        mProgramDescriptionTextView.setText(mProgramDescriptionText);
        measure(MeasureSpec.UNSPECIFIED, MeasureSpec.UNSPECIFIED);
        int targetHeight = getMeasuredHeight();

        if (mCurrentHeight == 0 || !isShown()) {
            // Do not add the resize animation when the banner has not been shown before.
            mCurrentHeight = targetHeight;
            LayoutParams layoutParams = (LayoutParams) getLayoutParams();
            if (targetHeight != layoutParams.height) {
                layoutParams.height = targetHeight;
                setLayoutParams(layoutParams);
            }
        } else if (mCurrentHeight != targetHeight || needProgramDescriptionFadeAnimation) {
            // Restore description text for fade in/out animation.
            if (needProgramDescriptionFadeAnimation) {
                mProgramDescriptionTextView.setText(oldDescription);
            }
            mResizeAnimator =
                    createResizeAnimator(targetHeight, needProgramDescriptionFadeAnimation);
            mResizeAnimator.start();
        }
    }

    private Animator createResizeAnimator(int targetHeight, boolean addFadeAnimation) {
        final ValueAnimator heightAnimator = ValueAnimator.ofInt(mCurrentHeight, targetHeight);
        heightAnimator.addUpdateListener(
                new ValueAnimator.AnimatorUpdateListener() {
                    @Override
                    public void onAnimationUpdate(ValueAnimator animation) {
                        int value = (Integer) animation.getAnimatedValue();
                        LayoutParams layoutParams =
                                (LayoutParams) ChannelBannerView.this.getLayoutParams();
                        if (value != layoutParams.height) {
                            layoutParams.height = value;
                            ChannelBannerView.this.setLayoutParams(layoutParams);
                        }
                        mCurrentHeight = value;
                    }
                });

        heightAnimator.setDuration(mResizeAnimDuration);
        heightAnimator.setInterpolator(mResizeInterpolator);

        if (!addFadeAnimation) {
            heightAnimator.addListener(mResizeAnimatorListener);
            return heightAnimator;
        }

        AnimatorSet fadeOutAndHeightAnimator = new AnimatorSet();
        fadeOutAndHeightAnimator.playTogether(mProgramDescriptionFadeOutAnimator, heightAnimator);
        AnimatorSet animator = new AnimatorSet();
        animator.playSequentially(fadeOutAndHeightAnimator, mProgramDescriptionFadeInAnimator);
        animator.addListener(mResizeAnimatorListener);
        return animator;
    }

    @Override
    public void onAccessibilityStateChanged(boolean enabled) {
        mAutoHideScheduler.onAccessibilityStateChanged(enabled);
    }

    //new add function ot others
    private String parseReslutionFromVideoFormat(String videoformat) {
        String resolution = null;
        if (videoformat != null && videoformat.split("_") != null && videoformat.split("_").length == 3 &&
                !mMainActivity.mQuickKeyInfo.isAtvSource()) {
            resolution = (videoformat.split("_"))[2];
        } else if (mMainActivity.mQuickKeyInfo.isAtvSource()) {
            String type = mCurrentChannel != null ? mCurrentChannel.getType() : null;
            if (TvContract.Channels.TYPE_PAL.equals(type)) {
                 int vfmt = 0;
                 if (mMainActivity.mQuickKeyInfo.getCurrentChannelInfo() != null) {
                    vfmt = mMainActivity.mQuickKeyInfo.getCurrentChannelInfo().getVfmt();
                }
                if ((vfmt & 0x00ffffff) == TvControlManager.V4L2_STD_PAL_M
                    || (vfmt & 0x00ffffff) == TvControlManager.V4L2_STD_PAL_60) {
                    resolution = ATVRESOLUTION_NTSC;
                } else {
                    resolution = ATVRESOLUTION_PAL;
                }
            } else if (TvContract.Channels.TYPE_NTSC.equals(type)) {
                resolution = ATVRESOLUTION_NTSC;
            } else if (TvContract.Channels.TYPE_SECAM.equals(type)) {
                resolution = ATVRESOLUTION_SECAM;
            }
        } else {
            resolution = EMPTY_STRING;
        }
        return resolution;
    }
}
