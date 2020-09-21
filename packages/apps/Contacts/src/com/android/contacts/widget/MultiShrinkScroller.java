package com.android.contacts.widget;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.animation.AnimatorListenerAdapter;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.drawable.GradientDrawable;
import android.hardware.display.DisplayManager;
import android.os.Trace;
import android.support.v4.view.ViewCompat;
import android.support.v4.view.animation.PathInterpolatorCompat;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.Display;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.animation.AnimationUtils;
import android.view.animation.Interpolator;
import android.widget.EdgeEffect;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Scroller;
import android.widget.TextView;
import android.widget.Toolbar;

import com.android.contacts.R;
import com.android.contacts.compat.CompatUtils;
import com.android.contacts.compat.EdgeEffectCompat;
import com.android.contacts.quickcontact.ExpandingEntryCardView;
import com.android.contacts.test.NeededForReflection;
import com.android.contacts.util.SchedulingUtils;

/**
 * A custom {@link ViewGroup} that operates similarly to a {@link ScrollView}, except with multiple
 * subviews. These subviews are scrolled or shrinked one at a time, until each reaches their
 * minimum or maximum value.
 *
 * MultiShrinkScroller is designed for a specific problem. As such, this class is designed to be
 * used with a specific layout file: quickcontact_activity.xml. MultiShrinkScroller expects subviews
 * with specific ID values.
 *
 * MultiShrinkScroller's code is heavily influenced by ScrollView. Nonetheless, several ScrollView
 * features are missing. For example: handling of KEYCODES, OverScroll bounce and saving
 * scroll state in savedInstanceState bundles.
 *
 * Before copying this approach to nested scrolling, consider whether something simpler & less
 * customized will work for you. For example, see the re-usable StickyHeaderListView used by
 * WifiSetupActivity (very nice). Alternatively, check out Google+'s cover photo scrolling or
 * Android L's built in nested scrolling support. I thought I needed a more custom ViewGroup in
 * order to track velocity, modify EdgeEffect color & perform the originally specified animations.
 * As a result this ViewGroup has non-standard talkback and keyboard support.
 */
public class MultiShrinkScroller extends FrameLayout {

    /**
     * 1000 pixels per second. Ie, 1 pixel per millisecond.
     */
    private static final int PIXELS_PER_SECOND = 1000;

    /**
     * Length of the acceleration animations. This value was taken from ValueAnimator.java.
     */
    private static final int EXIT_FLING_ANIMATION_DURATION_MS = 250;

    /**
     * In portrait mode, the height:width ratio of the photo's starting height.
     */
    private static final float INTERMEDIATE_HEADER_HEIGHT_RATIO = 0.6f;

    /**
     * Color blending will only be performed on the contact photo once the toolbar is compressed
     * to this ratio of its full height.
     */
    private static final float COLOR_BLENDING_START_RATIO = 0.5f;

    private static final float SPRING_DAMPENING_FACTOR = 0.01f;

    /**
     * When displaying a letter tile drawable, this alpha value should be used at the intermediate
     * toolbar height.
     */
    private static final float DESIRED_INTERMEDIATE_LETTER_TILE_ALPHA = 0.8f;

    private float[] mLastEventPosition = { 0, 0 };
    private VelocityTracker mVelocityTracker;
    private boolean mIsBeingDragged = false;
    private boolean mReceivedDown = false;
    /**
     * Did the current downwards fling/scroll-animation start while we were fullscreen?
     */
    private boolean mIsFullscreenDownwardsFling = false;

    private ScrollView mScrollView;
    private View mScrollViewChild;
    private View mToolbar;
    private QuickContactImageView mPhotoView;
    private View mPhotoViewContainer;
    private View mTransparentView;
    private MultiShrinkScrollerListener mListener;
    private TextView mFullNameView;
    private TextView mPhoneticNameView;
    private View mTitleAndPhoneticNameView;
    private View mPhotoTouchInterceptOverlay;
    /** Contains desired size & vertical offset of the title, once the header is fully compressed */
    private TextView mInvisiblePlaceholderTextView;
    private View mTitleGradientView;
    private View mActionBarGradientView;
    private View mStartColumn;
    private int mHeaderTintColor;
    private int mMaximumHeaderHeight;
    private int mMinimumHeaderHeight;
    /**
     * When the contact photo is tapped, it is resized to max size or this size. This value also
     * sometimes represents the maximum achievable header size achieved by scrolling. To enforce
     * this maximum in scrolling logic, always access this value via
     * {@link #getMaximumScrollableHeaderHeight}.
     */
    private int mIntermediateHeaderHeight;
    /**
     * If true, regular scrolling can expand the header beyond mIntermediateHeaderHeight. The
     * header, that contains the contact photo, can expand to a height equal its width.
     */
    private boolean mIsOpenContactSquare;
    private int mMaximumHeaderTextSize;
    private int mMaximumPhoneticNameViewHeight;
    private int mMaximumFullNameViewHeight;
    private int mCollapsedTitleBottomMargin;
    private int mCollapsedTitleStartMargin;
    private int mMinimumPortraitHeaderHeight;
    private int mMaximumPortraitHeaderHeight;
    /**
     * True once the header has touched the top of the screen at least once.
     */
    private boolean mHasEverTouchedTheTop;
    private boolean mIsTouchDisabledForDismissAnimation;
    private boolean mIsTouchDisabledForSuppressLayout;

    private final Scroller mScroller;
    private final EdgeEffect mEdgeGlowBottom;
    private final EdgeEffect mEdgeGlowTop;
    private final int mTouchSlop;
    private final int mMaximumVelocity;
    private final int mMinimumVelocity;
    private final int mDismissDistanceOnScroll;
    private final int mDismissDistanceOnRelease;
    private final int mSnapToTopSlopHeight;
    private final int mTransparentStartHeight;
    private final int mMaximumTitleMargin;
    private final float mToolbarElevation;
    private final boolean mIsTwoPanel;
    private final float mLandscapePhotoRatio;
    private final int mActionBarSize;

    // Objects used to perform color filtering on the header. These are stored as fields for
    // the sole purpose of avoiding "new" operations inside animation loops.
    private final ColorMatrix mWhitenessColorMatrix = new ColorMatrix();
    private final ColorMatrix mColorMatrix = new ColorMatrix();
    private final float[] mAlphaMatrixValues = {
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 1, 0
    };
    private final ColorMatrix mMultiplyBlendMatrix = new ColorMatrix();
    private final float[] mMultiplyBlendMatrixValues = {
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            0, 0, 0, 1, 0
    };

    private final Interpolator mTextSizePathInterpolator =
            PathInterpolatorCompat.create(0.16f, 0.4f, 0.2f, 1);

    private final int[] mGradientColors = new int[] {0,0x88000000};
    private GradientDrawable mTitleGradientDrawable = new GradientDrawable(
            GradientDrawable.Orientation.TOP_BOTTOM, mGradientColors);
    private GradientDrawable mActionBarGradientDrawable = new GradientDrawable(
            GradientDrawable.Orientation.BOTTOM_TOP, mGradientColors);

    public interface MultiShrinkScrollerListener {
        void onScrolledOffBottom();

        void onStartScrollOffBottom();

        void onTransparentViewHeightChange(float ratio);

        void onEntranceAnimationDone();

        void onEnterFullscreen();

        void onExitFullscreen();
    }

    private final AnimatorListener mSnapToBottomListener = new AnimatorListenerAdapter() {
        @Override
        public void onAnimationEnd(Animator animation) {
            if (getScrollUntilOffBottom() > 0 && mListener != null) {
                // Due to a rounding error, after the animation finished we haven't fully scrolled
                // off the screen. Lie to the listener: tell it that we did scroll off the screen.
                mListener.onScrolledOffBottom();
                // No other messages need to be sent to the listener.
                mListener = null;
            }
        }
    };

    /**
     * Interpolator from android.support.v4.view.ViewPager. Snappier and more elastic feeling
     * than the default interpolator.
     */
    private static final Interpolator sInterpolator = new Interpolator() {

        /**
         * {@inheritDoc}
         */
        @Override
        public float getInterpolation(float t) {
            t -= 1.0f;
            return t * t * t * t * t + 1.0f;
        }
    };

    public MultiShrinkScroller(Context context) {
        this(context, null);
    }

    public MultiShrinkScroller(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public MultiShrinkScroller(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        final ViewConfiguration configuration = ViewConfiguration.get(context);
        setFocusable(false);
        // Drawing must be enabled in order to support EdgeEffect
        setWillNotDraw(/* willNotDraw = */ false);

        mEdgeGlowBottom = new EdgeEffect(context);
        mEdgeGlowTop = new EdgeEffect(context);
        mScroller = new Scroller(context, sInterpolator);
        mTouchSlop = configuration.getScaledTouchSlop();
        mMinimumVelocity = configuration.getScaledMinimumFlingVelocity();
        mMaximumVelocity = configuration.getScaledMaximumFlingVelocity();
        mTransparentStartHeight = (int) getResources().getDimension(
                R.dimen.quickcontact_starting_empty_height);
        mToolbarElevation = getResources().getDimension(
                R.dimen.quick_contact_toolbar_elevation);
        mIsTwoPanel = getResources().getBoolean(R.bool.quickcontact_two_panel);
        mMaximumTitleMargin = (int) getResources().getDimension(
                R.dimen.quickcontact_title_initial_margin);

        mDismissDistanceOnScroll = (int) getResources().getDimension(
                R.dimen.quickcontact_dismiss_distance_on_scroll);
        mDismissDistanceOnRelease = (int) getResources().getDimension(
                R.dimen.quickcontact_dismiss_distance_on_release);
        mSnapToTopSlopHeight = (int) getResources().getDimension(
                R.dimen.quickcontact_snap_to_top_slop_height);

        final TypedValue photoRatio = new TypedValue();
        getResources().getValue(R.dimen.quickcontact_landscape_photo_ratio, photoRatio,
                            /* resolveRefs = */ true);
        mLandscapePhotoRatio = photoRatio.getFloat();

        final TypedArray attributeArray = context.obtainStyledAttributes(
                new int[]{android.R.attr.actionBarSize});
        mActionBarSize = attributeArray.getDimensionPixelSize(0, 0);
        mMinimumHeaderHeight = mActionBarSize;
        // This value is approximately equal to the portrait ActionBar size. It isn't exactly the
        // same, since the landscape and portrait ActionBar sizes can be different.
        mMinimumPortraitHeaderHeight = mMinimumHeaderHeight;
        attributeArray.recycle();
    }

    /**
     * This method must be called inside the Activity's OnCreate.
     */
    public void initialize(MultiShrinkScrollerListener listener, boolean isOpenContactSquare,
                final int maximumHeaderTextSize, final boolean shouldUpdateNameViewHeight) {
        mScrollView = (ScrollView) findViewById(R.id.content_scroller);
        mScrollViewChild = findViewById(R.id.card_container);
        mToolbar = findViewById(R.id.toolbar_parent);
        mPhotoViewContainer = findViewById(R.id.toolbar_parent);
        mTransparentView = findViewById(R.id.transparent_view);
        mFullNameView = (TextView) findViewById(R.id.large_title);
        mPhoneticNameView = (TextView) findViewById(R.id.phonetic_name);
        mTitleAndPhoneticNameView = findViewById(R.id.title_and_phonetic_name);
        mInvisiblePlaceholderTextView = (TextView) findViewById(R.id.placeholder_textview);
        mStartColumn = findViewById(R.id.empty_start_column);
        // Touching the empty space should close the card
        if (mStartColumn != null) {
            mStartColumn.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    scrollOffBottom();
                }
            });
            findViewById(R.id.empty_end_column).setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    scrollOffBottom();
                }
            });
        }
        mListener = listener;
        mIsOpenContactSquare = isOpenContactSquare;

        mPhotoView = (QuickContactImageView) findViewById(R.id.photo);

        mTitleGradientView = findViewById(R.id.title_gradient);
        mTitleGradientView.setBackground(mTitleGradientDrawable);
        mActionBarGradientView = findViewById(R.id.action_bar_gradient);
        mActionBarGradientView.setBackground(mActionBarGradientDrawable);
        mCollapsedTitleStartMargin = ((Toolbar) findViewById(R.id.toolbar)).getContentInsetStart();

        mPhotoTouchInterceptOverlay = findViewById(R.id.photo_touch_intercept_overlay);
        if (!mIsTwoPanel) {
            mPhotoTouchInterceptOverlay.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    expandHeader();
                }
            });
        }

        SchedulingUtils.doOnPreDraw(this, /* drawNextFrame = */ false, new Runnable() {
            @Override
            public void run() {
                if (!mIsTwoPanel) {
                    // We never want the height of the photo view to exceed its width.
                    mMaximumHeaderHeight = mPhotoViewContainer.getWidth();
                    mIntermediateHeaderHeight = (int) (mMaximumHeaderHeight
                            * INTERMEDIATE_HEADER_HEIGHT_RATIO);
                }
                mMaximumPortraitHeaderHeight = mIsTwoPanel ? getHeight()
                        : mPhotoViewContainer.getWidth();
                setHeaderHeight(getMaximumScrollableHeaderHeight());
                if (shouldUpdateNameViewHeight) {
                    mMaximumHeaderTextSize = mTitleAndPhoneticNameView.getHeight();
                    mMaximumFullNameViewHeight = mFullNameView.getHeight();
                    // We cannot rely on mPhoneticNameView.getHeight() since it could be 0
                    final int phoneticNameSize = getResources().getDimensionPixelSize(
                            R.dimen.quickcontact_maximum_phonetic_name_size);
                    final int fullNameSize = getResources().getDimensionPixelSize(
                            R.dimen.quickcontact_maximum_title_size);
                    mMaximumPhoneticNameViewHeight =
                            mMaximumFullNameViewHeight * phoneticNameSize / fullNameSize;
                }
                if (maximumHeaderTextSize > 0) {
                    mMaximumHeaderTextSize = maximumHeaderTextSize;
                }
                if (mIsTwoPanel) {
                    mMaximumHeaderHeight = getHeight();
                    mMinimumHeaderHeight = mMaximumHeaderHeight;
                    mIntermediateHeaderHeight = mMaximumHeaderHeight;

                    // Permanently set photo width and height.
                    final ViewGroup.LayoutParams photoLayoutParams
                            = mPhotoViewContainer.getLayoutParams();
                    photoLayoutParams.height = mMaximumHeaderHeight;
                    photoLayoutParams.width = (int) (mMaximumHeaderHeight * mLandscapePhotoRatio);
                    mPhotoViewContainer.setLayoutParams(photoLayoutParams);

                    // Permanently set title width and margin.
                    final FrameLayout.LayoutParams largeTextLayoutParams
                            = (FrameLayout.LayoutParams) mTitleAndPhoneticNameView
                            .getLayoutParams();
                    largeTextLayoutParams.width = photoLayoutParams.width -
                            largeTextLayoutParams.leftMargin - largeTextLayoutParams.rightMargin;
                    largeTextLayoutParams.gravity = Gravity.BOTTOM | Gravity.START;
                    mTitleAndPhoneticNameView.setLayoutParams(largeTextLayoutParams);
                } else {
                    // Set the width of mFullNameView as if it was nested inside
                    // mPhotoViewContainer.
                    mFullNameView.setWidth(mPhotoViewContainer.getWidth()
                            - 2 * mMaximumTitleMargin);
                    mPhoneticNameView.setWidth(mPhotoViewContainer.getWidth()
                            - 2 * mMaximumTitleMargin);
                }

                calculateCollapsedLargeTitlePadding();
                updateHeaderTextSizeAndMargin();
                configureGradientViewHeights();
            }
        });
    }

    private void configureGradientViewHeights() {
        final FrameLayout.LayoutParams actionBarGradientLayoutParams
                = (FrameLayout.LayoutParams) mActionBarGradientView.getLayoutParams();
        actionBarGradientLayoutParams.height = mActionBarSize;
        mActionBarGradientView.setLayoutParams(actionBarGradientLayoutParams);
        final FrameLayout.LayoutParams titleGradientLayoutParams
                = (FrameLayout.LayoutParams) mTitleGradientView.getLayoutParams();
        final float TITLE_GRADIENT_SIZE_COEFFICIENT = 1.25f;
        final FrameLayout.LayoutParams largeTextLayoutParms
                = (FrameLayout.LayoutParams) mTitleAndPhoneticNameView.getLayoutParams();
        titleGradientLayoutParams.height = (int) ((mMaximumHeaderTextSize
                + largeTextLayoutParms.bottomMargin) * TITLE_GRADIENT_SIZE_COEFFICIENT);
        mTitleGradientView.setLayoutParams(titleGradientLayoutParams);
    }

    public void setTitle(String title, boolean isPhoneNumber) {
        mFullNameView.setText(title);
        // We have a phone number as "mFullNameView" so make it always LTR.
        if (isPhoneNumber) {
            mFullNameView.setTextDirection(View.TEXT_DIRECTION_LTR);
        }
        mPhotoTouchInterceptOverlay.setContentDescription(title);
    }

    public void setPhoneticName(String phoneticName) {
        // Set phonetic name only when it was gone before or got changed.
        if (mPhoneticNameView.getVisibility() == View.VISIBLE
                && phoneticName.equals(mPhoneticNameView.getText())) {
            return;
        }
        mPhoneticNameView.setText(phoneticName);
        // Every time the phonetic name is changed, set mPhoneticNameView as visible,
        // in case it just changed from Visibility=GONE.
        mPhoneticNameView.setVisibility(View.VISIBLE);
        final int maximumHeaderTextSize =
                mMaximumFullNameViewHeight * mFullNameView.getLineCount()
                + mMaximumPhoneticNameViewHeight * mPhoneticNameView.getLineCount();
        // TODO try not using initialize() to refresh phonetic name view: b/27410518
        initialize(mListener, mIsOpenContactSquare, maximumHeaderTextSize,
                /* shouldUpdateNameViewHeight */ false);
    }

    public void setPhoneticNameGone() {
        // Remove phonetic name only when it was visible before.
        if (mPhoneticNameView.getVisibility() == View.GONE) {
            return;
        }
        mPhoneticNameView.setVisibility(View.GONE);
        final int maximumHeaderTextSize =
                mMaximumFullNameViewHeight * mFullNameView.getLineCount();
        // TODO try not using initialize() to refresh phonetic name view: b/27410518
        initialize(mListener, mIsOpenContactSquare, maximumHeaderTextSize,
                /* shouldUpdateNameViewHeight */ false);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        if (mVelocityTracker == null) {
            mVelocityTracker = VelocityTracker.obtain();
        }
        mVelocityTracker.addMovement(event);

        // The only time we want to intercept touch events is when we are being dragged.
        return shouldStartDrag(event);
    }

    private boolean shouldStartDrag(MotionEvent event) {
        if (mIsTouchDisabledForDismissAnimation || mIsTouchDisabledForSuppressLayout) return false;


        if (mIsBeingDragged) {
            mIsBeingDragged = false;
            return false;
        }

        switch (event.getAction()) {
            // If we are in the middle of a fling and there is a down event, we'll steal it and
            // start a drag.
            case MotionEvent.ACTION_DOWN:
                updateLastEventPosition(event);
                if (!mScroller.isFinished()) {
                    startDrag();
                    return true;
                } else {
                    mReceivedDown = true;
                }
                break;

            // Otherwise, we will start a drag if there is enough motion in the direction we are
            // capable of scrolling.
            case MotionEvent.ACTION_MOVE:
                if (motionShouldStartDrag(event)) {
                    updateLastEventPosition(event);
                    startDrag();
                    return true;
                }
                break;
        }

        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mIsTouchDisabledForDismissAnimation || mIsTouchDisabledForSuppressLayout) return true;

        final int action = event.getAction();

        if (mVelocityTracker == null) {
            mVelocityTracker = VelocityTracker.obtain();
        }
        mVelocityTracker.addMovement(event);

        if (!mIsBeingDragged) {
            if (shouldStartDrag(event)) {
                return true;
            }

            if (action == MotionEvent.ACTION_UP && mReceivedDown) {
                mReceivedDown = false;
                return performClick();
            }
            return true;
        }

        switch (action) {
            case MotionEvent.ACTION_MOVE:
                final float delta = updatePositionAndComputeDelta(event);
                scrollTo(0, getScroll() + (int) delta);
                mReceivedDown = false;

                if (mIsBeingDragged) {
                    final int distanceFromMaxScrolling = getMaximumScrollUpwards() - getScroll();
                    if (delta > distanceFromMaxScrolling) {
                        // The ScrollView is being pulled upwards while there is no more
                        // content offscreen, and the view port is already fully expanded.
                        EdgeEffectCompat.onPull(mEdgeGlowBottom, delta / getHeight(),
                                1 - event.getX() / getWidth());
                    }

                    if (!mEdgeGlowBottom.isFinished()) {
                        postInvalidateOnAnimation();
                    }

                    if (shouldDismissOnScroll()) {
                        scrollOffBottom();
                    }

                }
                break;

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                stopDrag(action == MotionEvent.ACTION_CANCEL);
                mReceivedDown = false;
                break;
        }

        return true;
    }

    public void setHeaderTintColor(int color) {
        mHeaderTintColor = color;
        updatePhotoTintAndDropShadow();
        if (CompatUtils.isLollipopCompatible()) {
            // Use the same amount of alpha on the new tint color as the previous tint color.
            final int edgeEffectAlpha = Color.alpha(mEdgeGlowBottom.getColor());
            mEdgeGlowBottom.setColor((color & 0xffffff) | Color.argb(edgeEffectAlpha, 0, 0, 0));
            mEdgeGlowTop.setColor(mEdgeGlowBottom.getColor());
        }
    }

    /**
     * Expand to maximum size.
     */
    private void expandHeader() {
        if (getHeaderHeight() != mMaximumHeaderHeight) {
            final ObjectAnimator animator = ObjectAnimator.ofInt(this, "headerHeight",
                    mMaximumHeaderHeight);
            animator.setDuration(ExpandingEntryCardView.DURATION_EXPAND_ANIMATION_CHANGE_BOUNDS);
            animator.start();
            // Scroll nested scroll view to its top
            if (mScrollView.getScrollY() != 0) {
                ObjectAnimator.ofInt(mScrollView, "scrollY", -mScrollView.getScrollY()).start();
            }
        }
    }

    private void startDrag() {
        mIsBeingDragged = true;
        mScroller.abortAnimation();
    }

    private void stopDrag(boolean cancelled) {
        mIsBeingDragged = false;
        if (!cancelled && getChildCount() > 0) {
            final float velocity = getCurrentVelocity();
            if (velocity > mMinimumVelocity || velocity < -mMinimumVelocity) {
                fling(-velocity);
                onDragFinished(mScroller.getFinalY() - mScroller.getStartY());
            } else {
                onDragFinished(/* flingDelta = */ 0);
            }
        } else {
            onDragFinished(/* flingDelta = */ 0);
        }

        if (mVelocityTracker != null) {
            mVelocityTracker.recycle();
            mVelocityTracker = null;
        }

        mEdgeGlowBottom.onRelease();
    }

    private void onDragFinished(int flingDelta) {
        if (getTransparentViewHeight() <= 0) {
            // Don't perform any snapping if quick contacts is full screen.
            return;
        }
        if (!snapToTopOnDragFinished(flingDelta)) {
            // The drag/fling won't result in the content at the top of the Window. Consider
            // snapping the content to the bottom of the window.
            snapToBottomOnDragFinished();
        }
    }

    /**
     * If needed, snap the subviews to the top of the Window.
     *
     * @return TRUE if QuickContacts will snap/fling to to top after this method call.
     */
    private boolean snapToTopOnDragFinished(int flingDelta) {
        if (!mHasEverTouchedTheTop) {
            // If the current fling is predicted to scroll past the top, then we don't need to snap
            // to the top. However, if the fling only flings past the top by a tiny amount,
            // it will look nicer to snap than to fling.
            final float predictedScrollPastTop = getTransparentViewHeight() - flingDelta;
            if (predictedScrollPastTop < -mSnapToTopSlopHeight) {
                return false;
            }

            if (getTransparentViewHeight() <= mTransparentStartHeight) {
                // We are above the starting scroll position so snap to the top.
                mScroller.forceFinished(true);
                smoothScrollBy(getTransparentViewHeight());
                return true;
            }
            return false;
        }
        if (getTransparentViewHeight() < mDismissDistanceOnRelease) {
            mScroller.forceFinished(true);
            smoothScrollBy(getTransparentViewHeight());
            return true;
        }
        return false;
    }

    /**
     * If needed, scroll all the subviews off the bottom of the Window.
     */
    private void snapToBottomOnDragFinished() {
        if (mHasEverTouchedTheTop) {
            if (getTransparentViewHeight() > mDismissDistanceOnRelease) {
                scrollOffBottom();
            }
            return;
        }
        if (getTransparentViewHeight() > mTransparentStartHeight) {
            scrollOffBottom();
        }
    }

    /**
     * Returns TRUE if we have scrolled far QuickContacts far enough that we should dismiss it
     * without waiting for the user to finish their drag.
     */
    private boolean shouldDismissOnScroll() {
        return mHasEverTouchedTheTop && getTransparentViewHeight() > mDismissDistanceOnScroll;
    }

    /**
     * Return ratio of non-transparent:viewgroup-height for this viewgroup at the starting position.
     */
    public float getStartingTransparentHeightRatio() {
        return getTransparentHeightRatio(mTransparentStartHeight);
    }

    private float getTransparentHeightRatio(int transparentHeight) {
        final float heightRatio = (float) transparentHeight / getHeight();
        // Clamp between [0, 1] in case this is called before height is initialized.
        return 1.0f - Math.max(Math.min(1.0f, heightRatio), 0f);
    }

    public void scrollOffBottom() {
        mIsTouchDisabledForDismissAnimation = true;
        final Interpolator interpolator = new AcceleratingFlingInterpolator(
                EXIT_FLING_ANIMATION_DURATION_MS, getCurrentVelocity(),
                getScrollUntilOffBottom());
        mScroller.forceFinished(true);
        ObjectAnimator translateAnimation = ObjectAnimator.ofInt(this, "scroll",
                getScroll() - getScrollUntilOffBottom());
        translateAnimation.setRepeatCount(0);
        translateAnimation.setInterpolator(interpolator);
        translateAnimation.setDuration(EXIT_FLING_ANIMATION_DURATION_MS);
        translateAnimation.addListener(mSnapToBottomListener);
        translateAnimation.start();
        if (mListener != null) {
            mListener.onStartScrollOffBottom();
        }
    }

    /**
     * @param scrollToCurrentPosition if true, will scroll from the bottom of the screen to the
     * current position. Otherwise, will scroll from the bottom of the screen to the top of the
     * screen.
     */
    public void scrollUpForEntranceAnimation(boolean scrollToCurrentPosition) {
        final int currentPosition = getScroll();
        final int bottomScrollPosition = currentPosition
                - (getHeight() - getTransparentViewHeight()) + 1;
        final Interpolator interpolator = AnimationUtils.loadInterpolator(getContext(),
                android.R.interpolator.linear_out_slow_in);
        final int desiredValue = currentPosition + (scrollToCurrentPosition ? currentPosition
                : getTransparentViewHeight());
        final ObjectAnimator animator = ObjectAnimator.ofInt(this, "scroll", bottomScrollPosition,
                desiredValue);
        animator.setInterpolator(interpolator);
        animator.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                if (animation.getAnimatedValue().equals(desiredValue) && mListener != null) {
                    mListener.onEntranceAnimationDone();
                }
            }
        });
        animator.start();
    }

    @Override
    public void scrollTo(int x, int y) {
        final int delta = y - getScroll();
        boolean wasFullscreen = getScrollNeededToBeFullScreen() <= 0;
        if (delta > 0) {
            scrollUp(delta);
        } else {
            scrollDown(delta);
        }
        updatePhotoTintAndDropShadow();
        updateHeaderTextSizeAndMargin();
        final boolean isFullscreen = getScrollNeededToBeFullScreen() <= 0;
        mHasEverTouchedTheTop |= isFullscreen;
        if (mListener != null) {
            if (wasFullscreen && !isFullscreen) {
                 mListener.onExitFullscreen();
            } else if (!wasFullscreen && isFullscreen) {
                mListener.onEnterFullscreen();
            }
            if (!isFullscreen || !wasFullscreen) {
                mListener.onTransparentViewHeightChange(
                        getTransparentHeightRatio(getTransparentViewHeight()));
            }
        }
    }

    /**
     * Change the height of the header/toolbar. Do *not* use this outside animations. This was
     * designed for use by {@link #prepareForShrinkingScrollChild}.
     */
    @NeededForReflection
    public void setToolbarHeight(int delta) {
        final ViewGroup.LayoutParams toolbarLayoutParams
                = mToolbar.getLayoutParams();
        toolbarLayoutParams.height = delta;
        mToolbar.setLayoutParams(toolbarLayoutParams);

        updatePhotoTintAndDropShadow();
        updateHeaderTextSizeAndMargin();
    }

    @NeededForReflection
    public int getToolbarHeight() {
        return mToolbar.getLayoutParams().height;
    }

    /**
     * Set the height of the toolbar and update its tint accordingly.
     */
    @NeededForReflection
    public void setHeaderHeight(int height) {
        final ViewGroup.LayoutParams toolbarLayoutParams
                = mToolbar.getLayoutParams();
        toolbarLayoutParams.height = height;
        mToolbar.setLayoutParams(toolbarLayoutParams);
        updatePhotoTintAndDropShadow();
        updateHeaderTextSizeAndMargin();
    }

    @NeededForReflection
    public int getHeaderHeight() {
        return mToolbar.getLayoutParams().height;
    }

    @NeededForReflection
    public void setScroll(int scroll) {
        scrollTo(0, scroll);
    }

    /**
     * Returns the total amount scrolled inside the nested ScrollView + the amount of shrinking
     * performed on the ToolBar. This is the value inspected by animators.
     */
    @NeededForReflection
    public int getScroll() {
        return mTransparentStartHeight - getTransparentViewHeight()
                + getMaximumScrollableHeaderHeight() - getToolbarHeight()
                + mScrollView.getScrollY();
    }

    private int getMaximumScrollableHeaderHeight() {
        return mIsOpenContactSquare ? mMaximumHeaderHeight : mIntermediateHeaderHeight;
    }

    /**
     * A variant of {@link #getScroll} that pretends the header is never larger than
     * than mIntermediateHeaderHeight. This function is sometimes needed when making scrolling
     * decisions that will not change the header size (ie, snapping to the bottom or top).
     *
     * When mIsOpenContactSquare is true, this function considers mIntermediateHeaderHeight ==
     * mMaximumHeaderHeight, since snapping decisions will be made relative the full header
     * size when mIsOpenContactSquare = true.
     *
     * This value should never be used in conjunction with {@link #getScroll} values.
     */
    private int getScroll_ignoreOversizedHeaderForSnapping() {
        return mTransparentStartHeight - getTransparentViewHeight()
                + Math.max(getMaximumScrollableHeaderHeight() - getToolbarHeight(), 0)
                + mScrollView.getScrollY();
    }

    /**
     * Amount of transparent space above the header/toolbar.
     */
    public int getScrollNeededToBeFullScreen() {
        return getTransparentViewHeight();
    }

    /**
     * Return amount of scrolling needed in order for all the visible subviews to scroll off the
     * bottom.
     */
    private int getScrollUntilOffBottom() {
        return getHeight() + getScroll_ignoreOversizedHeaderForSnapping()
                - mTransparentStartHeight;
    }

    @Override
    public void computeScroll() {
        if (mScroller.computeScrollOffset()) {
            // Examine the fling results in order to activate EdgeEffect and halt flings.
            final int oldScroll = getScroll();
            scrollTo(0, mScroller.getCurrY());
            final int delta = mScroller.getCurrY() - oldScroll;
            final int distanceFromMaxScrolling = getMaximumScrollUpwards() - getScroll();
            if (delta > distanceFromMaxScrolling && distanceFromMaxScrolling > 0) {
                mEdgeGlowBottom.onAbsorb((int) mScroller.getCurrVelocity());
            }
            if (mIsFullscreenDownwardsFling && getTransparentViewHeight() > 0) {
                // Halt the fling once QuickContact's top is on screen.
                scrollTo(0, getScroll() + getTransparentViewHeight());
                mEdgeGlowTop.onAbsorb((int) mScroller.getCurrVelocity());
                mScroller.abortAnimation();
                mIsFullscreenDownwardsFling = false;
            }
            if (!awakenScrollBars()) {
                // Keep on drawing until the animation has finished.
                postInvalidateOnAnimation();
            }
            if (mScroller.getCurrY() >= getMaximumScrollUpwards()) {
                // Halt the fling once QuickContact's bottom is on screen.
                mScroller.abortAnimation();
                mIsFullscreenDownwardsFling = false;
            }
        }
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);

        final int width = getWidth() - getPaddingLeft() - getPaddingRight();
        final int height = getHeight();

        if (!mEdgeGlowBottom.isFinished()) {
            final int restoreCount = canvas.save();

            // Draw the EdgeEffect on the bottom of the Window (Or a little bit below the bottom
            // of the Window if we start to scroll upwards while EdgeEffect is visible). This
            // does not need to consider the case where this MultiShrinkScroller doesn't fill
            // the Window, since the nested ScrollView should be set to fillViewport.
            canvas.translate(-width + getPaddingLeft(),
                    height + getMaximumScrollUpwards() - getScroll());

            canvas.rotate(180, width, 0);
            if (mIsTwoPanel) {
                // Only show the EdgeEffect on the bottom of the ScrollView.
                mEdgeGlowBottom.setSize(mScrollView.getWidth(), height);
                if (getLayoutDirection() == View.LAYOUT_DIRECTION_RTL) {
                    canvas.translate(mPhotoViewContainer.getWidth(), 0);
                }
            } else {
                mEdgeGlowBottom.setSize(width, height);
            }
            if (mEdgeGlowBottom.draw(canvas)) {
                postInvalidateOnAnimation();
            }
            canvas.restoreToCount(restoreCount);
        }

        if (!mEdgeGlowTop.isFinished()) {
            final int restoreCount = canvas.save();
            if (mIsTwoPanel) {
                mEdgeGlowTop.setSize(mScrollView.getWidth(), height);
                if (getLayoutDirection() != View.LAYOUT_DIRECTION_RTL) {
                    canvas.translate(mPhotoViewContainer.getWidth(), 0);
                }
            } else {
                mEdgeGlowTop.setSize(width, height);
            }
            if (mEdgeGlowTop.draw(canvas)) {
                postInvalidateOnAnimation();
            }
            canvas.restoreToCount(restoreCount);
        }
    }

    private float getCurrentVelocity() {
        if (mVelocityTracker == null) {
            return 0;
        }
        mVelocityTracker.computeCurrentVelocity(PIXELS_PER_SECOND, mMaximumVelocity);
        return mVelocityTracker.getYVelocity();
    }

    private void fling(float velocity) {
        // For reasons I do not understand, scrolling is less janky when maxY=Integer.MAX_VALUE
        // then when maxY is set to an actual value.
        mScroller.fling(0, getScroll(), 0, (int) velocity, 0, 0, -Integer.MAX_VALUE,
                Integer.MAX_VALUE);
        if (velocity < 0 && mTransparentView.getHeight() <= 0) {
            mIsFullscreenDownwardsFling = true;
        }
        invalidate();
    }

    private int getMaximumScrollUpwards() {
        if (!mIsTwoPanel) {
            return mTransparentStartHeight
                    // How much the Header view can compress
                    + getMaximumScrollableHeaderHeight() - getFullyCompressedHeaderHeight()
                    // How much the ScrollView can scroll. 0, if child is smaller than ScrollView.
                    + Math.max(0, mScrollViewChild.getHeight() - getHeight()
                    + getFullyCompressedHeaderHeight());
        } else {
            return mTransparentStartHeight
                    // How much the ScrollView can scroll. 0, if child is smaller than ScrollView.
                    + Math.max(0, mScrollViewChild.getHeight() - getHeight());
        }
    }

    private int getTransparentViewHeight() {
        return mTransparentView.getLayoutParams().height;
    }

    private void setTransparentViewHeight(int height) {
        mTransparentView.getLayoutParams().height = height;
        mTransparentView.setLayoutParams(mTransparentView.getLayoutParams());
    }

    private void scrollUp(int delta) {
        if (getTransparentViewHeight() != 0) {
            final int originalValue = getTransparentViewHeight();
            setTransparentViewHeight(getTransparentViewHeight() - delta);
            setTransparentViewHeight(Math.max(0, getTransparentViewHeight()));
            delta -= originalValue - getTransparentViewHeight();
        }
        final ViewGroup.LayoutParams toolbarLayoutParams
                = mToolbar.getLayoutParams();
        if (toolbarLayoutParams.height > getFullyCompressedHeaderHeight()) {
            final int originalValue = toolbarLayoutParams.height;
            toolbarLayoutParams.height -= delta;
            toolbarLayoutParams.height = Math.max(toolbarLayoutParams.height,
                    getFullyCompressedHeaderHeight());
            mToolbar.setLayoutParams(toolbarLayoutParams);
            delta -= originalValue - toolbarLayoutParams.height;
        }
        mScrollView.scrollBy(0, delta);
    }

    /**
     * Returns the minimum size that we want to compress the header to, given that we don't want to
     * allow the the ScrollView to scroll unless there is new content off of the edge of ScrollView.
     */
    private int getFullyCompressedHeaderHeight() {
        return Math.min(Math.max(mToolbar.getLayoutParams().height - getOverflowingChildViewSize(),
                mMinimumHeaderHeight), getMaximumScrollableHeaderHeight());
    }

    /**
     * Returns the amount of mScrollViewChild that doesn't fit inside its parent.
     */
    private int getOverflowingChildViewSize() {
        final int usedScrollViewSpace = mScrollViewChild.getHeight();
        return -getHeight() + usedScrollViewSpace + mToolbar.getLayoutParams().height;
    }

    private void scrollDown(int delta) {
        if (mScrollView.getScrollY() > 0) {
            final int originalValue = mScrollView.getScrollY();
            mScrollView.scrollBy(0, delta);
            delta -= mScrollView.getScrollY() - originalValue;
        }
        final ViewGroup.LayoutParams toolbarLayoutParams = mToolbar.getLayoutParams();
        if (toolbarLayoutParams.height < getMaximumScrollableHeaderHeight()) {
            final int originalValue = toolbarLayoutParams.height;
            toolbarLayoutParams.height -= delta;
            toolbarLayoutParams.height = Math.min(toolbarLayoutParams.height,
                    getMaximumScrollableHeaderHeight());
            mToolbar.setLayoutParams(toolbarLayoutParams);
            delta -= originalValue - toolbarLayoutParams.height;
        }
        setTransparentViewHeight(getTransparentViewHeight() - delta);

        if (getScrollUntilOffBottom() <= 0) {
            post(new Runnable() {
                @Override
                public void run() {
                    if (mListener != null) {
                        mListener.onScrolledOffBottom();
                        // No other messages need to be sent to the listener.
                        mListener = null;
                    }
                }
            });
        }
    }

    /**
     * Set the header size and padding, based on the current scroll position.
     */
    private void updateHeaderTextSizeAndMargin() {
        if (mIsTwoPanel) {
            // The text size stays at a constant size & location in two panel layouts.
            return;
        }

        // The pivot point for scaling should be middle of the starting side.
        if (getLayoutDirection() == View.LAYOUT_DIRECTION_RTL) {
            mTitleAndPhoneticNameView.setPivotX(mTitleAndPhoneticNameView.getWidth());
        } else {
            mTitleAndPhoneticNameView.setPivotX(0);
        }
        mTitleAndPhoneticNameView.setPivotY(mMaximumHeaderTextSize / 2);

        final int toolbarHeight = mToolbar.getLayoutParams().height;
        mPhotoTouchInterceptOverlay.setClickable(toolbarHeight != mMaximumHeaderHeight);

        if (toolbarHeight >= mMaximumHeaderHeight) {
            // Everything is full size when the header is fully expanded.
            mTitleAndPhoneticNameView.setScaleX(1);
            mTitleAndPhoneticNameView.setScaleY(1);
            setInterpolatedTitleMargins(1);
            return;
        }

        final float ratio = (toolbarHeight  - mMinimumHeaderHeight)
                / (float)(mMaximumHeaderHeight - mMinimumHeaderHeight);
        final float minimumSize = mInvisiblePlaceholderTextView.getHeight();
        float bezierOutput = mTextSizePathInterpolator.getInterpolation(ratio);
        float scale = (minimumSize + (mMaximumHeaderTextSize - minimumSize) * bezierOutput)
                / mMaximumHeaderTextSize;

        // Clamp to reasonable/finite values before passing into framework. The values
        // can be wacky before the first pre-render.
        bezierOutput = (float) Math.min(bezierOutput, 1.0f);
        scale = (float) Math.min(scale, 1.0f);

        mTitleAndPhoneticNameView.setScaleX(scale);
        mTitleAndPhoneticNameView.setScaleY(scale);
        setInterpolatedTitleMargins(bezierOutput);
    }

    /**
     * Calculate the padding around mTitleAndPhoneticNameView so that it will look appropriate once it
     * finishes moving into its target location/size.
     */
    private void calculateCollapsedLargeTitlePadding() {
        int invisiblePlaceHolderLocation[] = new int[2];
        int largeTextViewRectLocation[] = new int[2];
        mInvisiblePlaceholderTextView.getLocationOnScreen(invisiblePlaceHolderLocation);
        mToolbar.getLocationOnScreen(largeTextViewRectLocation);
        // Distance between top of toolbar to the center of the target rectangle.
        final int desiredTopToCenter = invisiblePlaceHolderLocation[1]
                + mInvisiblePlaceholderTextView.getHeight() / 2
                - largeTextViewRectLocation[1];
        // Padding needed on the mTitleAndPhoneticNameView so that it has the same amount of
        // padding as the target rectangle.
        mCollapsedTitleBottomMargin =
                desiredTopToCenter - mMaximumHeaderTextSize / 2;
    }

    /**
     * Interpolate the title's margin size. When {@param x}=1, use the maximum title margins.
     * When {@param x}=0, use the margin values taken from {@link #mInvisiblePlaceholderTextView}.
     */
    private void setInterpolatedTitleMargins(float x) {
        final FrameLayout.LayoutParams titleLayoutParams
                = (FrameLayout.LayoutParams) mTitleAndPhoneticNameView.getLayoutParams();
        final LinearLayout.LayoutParams toolbarLayoutParams
                = (LinearLayout.LayoutParams) mToolbar.getLayoutParams();

        // Need to add more to margin start if there is a start column
        int startColumnWidth = mStartColumn == null ? 0 : mStartColumn.getWidth();

        titleLayoutParams.setMarginStart((int) (mCollapsedTitleStartMargin * (1 - x)
                + mMaximumTitleMargin * x) + startColumnWidth);
        // How offset the title should be from the bottom of the toolbar
        final int pretendBottomMargin =  (int) (mCollapsedTitleBottomMargin * (1 - x)
                + mMaximumTitleMargin * x) ;
        // Calculate how offset the title should be from the top of the screen. Instead of
        // calling mTitleAndPhoneticNameView.getHeight() use the mMaximumHeaderTextSize for this
        // calculation. The getHeight() value acts unexpectedly when mTitleAndPhoneticNameView is
        // partially clipped by its parent.
        titleLayoutParams.topMargin = getTransparentViewHeight()
                + toolbarLayoutParams.height - pretendBottomMargin
                - mMaximumHeaderTextSize;
        titleLayoutParams.bottomMargin = 0;
        mTitleAndPhoneticNameView.setLayoutParams(titleLayoutParams);
    }

    private void updatePhotoTintAndDropShadow() {
        // Let's keep an eye on how long this method takes to complete.
        Trace.beginSection("updatePhotoTintAndDropShadow");

        // Tell the photo view what tint we are trying to achieve. Depending on the type of
        // drawable used, the photo view may or may not use this tint.
        mPhotoView.setTint(mHeaderTintColor);

        if (mIsTwoPanel && !mPhotoView.isBasedOffLetterTile()) {
            // When in two panel mode, UX considers photo tinting unnecessary for non letter
            // tile photos.
            mTitleGradientDrawable.setAlpha(0xFF);
            mActionBarGradientDrawable.setAlpha(0xFF);
            return;
        }

        // We need to use toolbarLayoutParams to determine the height, since the layout
        // params can be updated before the height change is reflected inside the View#getHeight().
        final int toolbarHeight = getToolbarHeight();

        if (toolbarHeight <= mMinimumHeaderHeight && !mIsTwoPanel) {
            ViewCompat.setElevation(mPhotoViewContainer, mToolbarElevation);
        } else {
            ViewCompat.setElevation(mPhotoViewContainer, 0);
        }

        // Reuse an existing mColorFilter (to avoid GC pauses) to change the photo's tint.
        mPhotoView.clearColorFilter();
        mColorMatrix.reset();

        final int gradientAlpha;
        if (!mPhotoView.isBasedOffLetterTile()) {
            // Constants and equations were arbitrarily picked to choose values for saturation,
            // whiteness, tint and gradient alpha. There were four main objectives:
            // 1) The transition period between the unmodified image and fully colored image should
            //    be very short.
            // 2) The tinting should be fully applied even before the background image is fully
            //    faded out and desaturated. Why? A half tinted photo looks bad and results in
            //    unappealing colors.
            // 3) The function should have a derivative of 0 at ratio = 1 to avoid discontinuities.
            // 4) The entire process should look awesome.
            final float ratio = calculateHeightRatioToBlendingStartHeight(toolbarHeight);
            final float alpha = 1.0f - (float) Math.min(Math.pow(ratio, 1.5f) * 2f, 1f);
            final float tint = (float) Math.min(Math.pow(ratio, 1.5f) * 3f, 1f);
            mColorMatrix.setSaturation(alpha);
            mColorMatrix.postConcat(alphaMatrix(alpha, Color.WHITE));
            mColorMatrix.postConcat(multiplyBlendMatrix(mHeaderTintColor, tint));
            gradientAlpha = (int) (255 * alpha);
        } else if (mIsTwoPanel) {
            mColorMatrix.reset();
            mColorMatrix.postConcat(alphaMatrix(DESIRED_INTERMEDIATE_LETTER_TILE_ALPHA,
                    mHeaderTintColor));
            gradientAlpha = 0;
        } else {
            // We want a function that has DESIRED_INTERMEDIATE_LETTER_TILE_ALPHA value
            // at the intermediate position and uses TILE_EXPONENT. Finding an equation
            // that satisfies this condition requires the following arithmetic.
            final float ratio = calculateHeightRatioToFullyOpen(toolbarHeight);
            final float intermediateRatio = calculateHeightRatioToFullyOpen((int)
                    (mMaximumPortraitHeaderHeight * INTERMEDIATE_HEADER_HEIGHT_RATIO));
            final float TILE_EXPONENT = 3f;
            final float slowingFactor = (float) ((1 - intermediateRatio) / intermediateRatio
                    / (1 - Math.pow(1 - DESIRED_INTERMEDIATE_LETTER_TILE_ALPHA, 1/TILE_EXPONENT)));
            float linearBeforeIntermediate = Math.max(1 - (1 - ratio) / intermediateRatio
                    / slowingFactor, 0);
            float colorAlpha = 1 - (float) Math.pow(linearBeforeIntermediate, TILE_EXPONENT);
            mColorMatrix.postConcat(alphaMatrix(colorAlpha, mHeaderTintColor));
            gradientAlpha = 0;
        }

        // TODO: remove re-allocation of ColorMatrixColorFilter objects (b/17627000)
        mPhotoView.setColorFilter(new ColorMatrixColorFilter(mColorMatrix));

        mTitleGradientDrawable.setAlpha(gradientAlpha);
        mActionBarGradientDrawable.setAlpha(gradientAlpha);

        Trace.endSection();
    }

    private float calculateHeightRatioToFullyOpen(int height) {
        return (height - mMinimumPortraitHeaderHeight)
                / (float) (mMaximumPortraitHeaderHeight - mMinimumPortraitHeaderHeight);
    }

    private float calculateHeightRatioToBlendingStartHeight(int height) {
        final float intermediateHeight = mMaximumPortraitHeaderHeight
                * COLOR_BLENDING_START_RATIO;
        final float interpolatingHeightRange = intermediateHeight - mMinimumPortraitHeaderHeight;
        if (height > intermediateHeight) {
            return 0;
        }
        return (intermediateHeight - height) / interpolatingHeightRange;
    }

    /**
     * Simulates alpha blending an image with {@param color}.
     */
    private ColorMatrix alphaMatrix(float alpha, int color) {
        mAlphaMatrixValues[0] = Color.red(color) * alpha / 255;
        mAlphaMatrixValues[6] = Color.green(color) * alpha / 255;
        mAlphaMatrixValues[12] = Color.blue(color) * alpha / 255;
        mAlphaMatrixValues[4] = 255 * (1 - alpha);
        mAlphaMatrixValues[9] = 255 * (1 - alpha);
        mAlphaMatrixValues[14] = 255 * (1 - alpha);
        mWhitenessColorMatrix.set(mAlphaMatrixValues);
        return mWhitenessColorMatrix;
    }

    /**
     * Simulates multiply blending an image with a single {@param color}.
     *
     * Multiply blending is [Sa * Da, Sc * Dc]. See {@link android.graphics.PorterDuff}.
     */
    private ColorMatrix multiplyBlendMatrix(int color, float alpha) {
        mMultiplyBlendMatrixValues[0] = multiplyBlend(Color.red(color), alpha);
        mMultiplyBlendMatrixValues[6] = multiplyBlend(Color.green(color), alpha);
        mMultiplyBlendMatrixValues[12] = multiplyBlend(Color.blue(color), alpha);
        mMultiplyBlendMatrix.set(mMultiplyBlendMatrixValues);
        return mMultiplyBlendMatrix;
    }

    private float multiplyBlend(int color, float alpha) {
        return color * alpha / 255.0f + (1 - alpha);
    }

    private void updateLastEventPosition(MotionEvent event) {
        mLastEventPosition[0] = event.getX();
        mLastEventPosition[1] = event.getY();
    }

    private boolean motionShouldStartDrag(MotionEvent event) {
        final float deltaY = event.getY() - mLastEventPosition[1];
        return deltaY > mTouchSlop || deltaY < -mTouchSlop;
    }

    private float updatePositionAndComputeDelta(MotionEvent event) {
        final int VERTICAL = 1;
        final float position = mLastEventPosition[VERTICAL];
        updateLastEventPosition(event);
        float elasticityFactor = 1;
        if (position < mLastEventPosition[VERTICAL] && mHasEverTouchedTheTop) {
            // As QuickContacts is dragged from the top of the window, its rate of movement will
            // slow down in proportion to its distance from the top. This will feel springy.
            elasticityFactor += mTransparentView.getHeight() * SPRING_DAMPENING_FACTOR;
        }
        return (position - mLastEventPosition[VERTICAL]) / elasticityFactor;
    }

    private void smoothScrollBy(int delta) {
        if (delta == 0) {
            // Delta=0 implies the code calling smoothScrollBy is sloppy. We should avoid doing
            // this, since it prevents Views from being able to register any clicks for 250ms.
            throw new IllegalArgumentException("Smooth scrolling by delta=0 is "
                    + "pointless and harmful");
        }
        mScroller.startScroll(0, getScroll(), 0, delta);
        invalidate();
    }

    /**
     * Interpolator that enforces a specific starting velocity. This is useful to avoid a
     * discontinuity between dragging speed and flinging speed.
     *
     * Similar to a {@link android.view.animation.AccelerateInterpolator} in the sense that
     * getInterpolation() is a quadratic function.
     */
    private class AcceleratingFlingInterpolator implements Interpolator {

        private final float mStartingSpeedPixelsPerFrame;
        private final float mDurationMs;
        private final int mPixelsDelta;
        private final float mNumberFrames;

        public AcceleratingFlingInterpolator(int durationMs, float startingSpeedPixelsPerSecond,
                int pixelsDelta) {
            mStartingSpeedPixelsPerFrame = startingSpeedPixelsPerSecond / getRefreshRate();
            mDurationMs = durationMs;
            mPixelsDelta = pixelsDelta;
            mNumberFrames = mDurationMs / getFrameIntervalMs();
        }

        @Override
        public float getInterpolation(float input) {
            final float animationIntervalNumber = mNumberFrames * input;
            final float linearDelta = (animationIntervalNumber * mStartingSpeedPixelsPerFrame)
                    / mPixelsDelta;
            // Add the results of a linear interpolator (with the initial speed) with the
            // results of a AccelerateInterpolator.
            if (mStartingSpeedPixelsPerFrame > 0) {
                return Math.min(input * input + linearDelta, 1);
            } else {
                // Initial fling was in the wrong direction, make sure that the quadratic component
                // grows faster in order to make up for this.
                return Math.min(input * (input - linearDelta) + linearDelta, 1);
            }
        }

        private float getRefreshRate() {
            final DisplayManager displayManager = (DisplayManager) MultiShrinkScroller
                    .this.getContext().getSystemService(Context.DISPLAY_SERVICE);
            return displayManager.getDisplay(Display.DEFAULT_DISPLAY).getRefreshRate();
        }

        public long getFrameIntervalMs() {
            return (long)(1000 / getRefreshRate());
        }
    }

    /**
     * Expand the header if the mScrollViewChild is about to shrink by enough to create new empty
     * space at the bottom of this ViewGroup.
     */
    public void prepareForShrinkingScrollChild(int heightDelta) {
        final int newEmptyScrollViewSpace = -getOverflowingChildViewSize() + heightDelta;
        if (newEmptyScrollViewSpace > 0 && !mIsTwoPanel) {
            final int newDesiredToolbarHeight = Math.min(getToolbarHeight()
                    + newEmptyScrollViewSpace, getMaximumScrollableHeaderHeight());
            ObjectAnimator.ofInt(this, "toolbarHeight", newDesiredToolbarHeight).setDuration(
                    ExpandingEntryCardView.DURATION_COLLAPSE_ANIMATION_CHANGE_BOUNDS).start();
        }
    }

    /**
     * If {@param areTouchesDisabled} is TRUE, ignore all of the user's touches.
     */
    public void setDisableTouchesForSuppressLayout(boolean areTouchesDisabled) {
        // The card expansion animation uses the Transition framework's ChangeBounds API. This
        // invokes suppressLayout(true) on the MultiShrinkScroller. As a result, we need to avoid
        // all layout changes during expansion in order to avoid weird layout artifacts.
        mIsTouchDisabledForSuppressLayout = areTouchesDisabled;
    }
}
