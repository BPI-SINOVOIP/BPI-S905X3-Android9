/*
 * Copyright (C) 2006 The Android Open Source Project
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

package android.view;

import static android.view.WindowManagerPolicyConstants.APPLICATION_MEDIA_OVERLAY_SUBLAYER;
import static android.view.WindowManagerPolicyConstants.APPLICATION_MEDIA_SUBLAYER;
import static android.view.WindowManagerPolicyConstants.APPLICATION_PANEL_SUBLAYER;

import android.content.Context;
import android.content.res.CompatibilityInfo.Translator;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.Region;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.util.Log;

import com.android.internal.view.SurfaceCallbackHelper;

import java.util.ArrayList;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Provides a dedicated drawing surface embedded inside of a view hierarchy.
 * You can control the format of this surface and, if you like, its size; the
 * SurfaceView takes care of placing the surface at the correct location on the
 * screen
 *
 * <p>The surface is Z ordered so that it is behind the window holding its
 * SurfaceView; the SurfaceView punches a hole in its window to allow its
 * surface to be displayed. The view hierarchy will take care of correctly
 * compositing with the Surface any siblings of the SurfaceView that would
 * normally appear on top of it. This can be used to place overlays such as
 * buttons on top of the Surface, though note however that it can have an
 * impact on performance since a full alpha-blended composite will be performed
 * each time the Surface changes.
 *
 * <p> The transparent region that makes the surface visible is based on the
 * layout positions in the view hierarchy. If the post-layout transform
 * properties are used to draw a sibling view on top of the SurfaceView, the
 * view may not be properly composited with the surface.
 *
 * <p>Access to the underlying surface is provided via the SurfaceHolder interface,
 * which can be retrieved by calling {@link #getHolder}.
 *
 * <p>The Surface will be created for you while the SurfaceView's window is
 * visible; you should implement {@link SurfaceHolder.Callback#surfaceCreated}
 * and {@link SurfaceHolder.Callback#surfaceDestroyed} to discover when the
 * Surface is created and destroyed as the window is shown and hidden.
 *
 * <p>One of the purposes of this class is to provide a surface in which a
 * secondary thread can render into the screen. If you are going to use it
 * this way, you need to be aware of some threading semantics:
 *
 * <ul>
 * <li> All SurfaceView and
 * {@link SurfaceHolder.Callback SurfaceHolder.Callback} methods will be called
 * from the thread running the SurfaceView's window (typically the main thread
 * of the application). They thus need to correctly synchronize with any
 * state that is also touched by the drawing thread.
 * <li> You must ensure that the drawing thread only touches the underlying
 * Surface while it is valid -- between
 * {@link SurfaceHolder.Callback#surfaceCreated SurfaceHolder.Callback.surfaceCreated()}
 * and
 * {@link SurfaceHolder.Callback#surfaceDestroyed SurfaceHolder.Callback.surfaceDestroyed()}.
 * </ul>
 *
 * <p class="note"><strong>Note:</strong> Starting in platform version
 * {@link android.os.Build.VERSION_CODES#N}, SurfaceView's window position is
 * updated synchronously with other View rendering. This means that translating
 * and scaling a SurfaceView on screen will not cause rendering artifacts. Such
 * artifacts may occur on previous versions of the platform when its window is
 * positioned asynchronously.</p>
 */
public class SurfaceView extends View implements ViewRootImpl.WindowStoppedCallback {
    private static final String TAG = "SurfaceView";
    private static final boolean DEBUG = false;

    final ArrayList<SurfaceHolder.Callback> mCallbacks
            = new ArrayList<SurfaceHolder.Callback>();

    final int[] mLocation = new int[2];

    final ReentrantLock mSurfaceLock = new ReentrantLock();
    final Surface mSurface = new Surface();       // Current surface in use
    boolean mDrawingStopped = true;
    // We use this to track if the application has produced a frame
    // in to the Surface. Up until that point, we should be careful not to punch
    // holes.
    boolean mDrawFinished = false;

    final Rect mScreenRect = new Rect();
    SurfaceSession mSurfaceSession;

    SurfaceControlWithBackground mSurfaceControl;
    // In the case of format changes we switch out the surface in-place
    // we need to preserve the old one until the new one has drawn.
    SurfaceControl mDeferredDestroySurfaceControl;
    final Rect mTmpRect = new Rect();
    final Configuration mConfiguration = new Configuration();

    int mSubLayer = APPLICATION_MEDIA_SUBLAYER;

    boolean mIsCreating = false;
    private volatile boolean mRtHandlingPositionUpdates = false;

    private final ViewTreeObserver.OnScrollChangedListener mScrollChangedListener
            = new ViewTreeObserver.OnScrollChangedListener() {
                    @Override
                    public void onScrollChanged() {
                        updateSurface();
                    }
            };

    private final ViewTreeObserver.OnPreDrawListener mDrawListener =
            new ViewTreeObserver.OnPreDrawListener() {
                @Override
                public boolean onPreDraw() {
                    // reposition ourselves where the surface is
                    mHaveFrame = getWidth() > 0 && getHeight() > 0;
                    updateSurface();
                    return true;
                }
            };

    boolean mRequestedVisible = false;
    boolean mWindowVisibility = false;
    boolean mLastWindowVisibility = false;
    boolean mViewVisibility = false;
    boolean mWindowStopped = false;

    int mRequestedWidth = -1;
    int mRequestedHeight = -1;
    /* Set SurfaceView's format to 565 by default to maintain backward
     * compatibility with applications assuming this format.
     */
    int mRequestedFormat = PixelFormat.RGB_565;

    boolean mHaveFrame = false;
    boolean mSurfaceCreated = false;
    long mLastLockTime = 0;

    boolean mVisible = false;
    int mWindowSpaceLeft = -1;
    int mWindowSpaceTop = -1;
    int mSurfaceWidth = -1;
    int mSurfaceHeight = -1;
    int mFormat = -1;
    final Rect mSurfaceFrame = new Rect();
    int mLastSurfaceWidth = -1, mLastSurfaceHeight = -1;
    private Translator mTranslator;

    private boolean mGlobalListenersAdded;
    private boolean mAttachedToWindow;

    private int mSurfaceFlags = SurfaceControl.HIDDEN;

    private int mPendingReportDraws;

    private SurfaceControl.Transaction mRtTransaction = new SurfaceControl.Transaction();

    public SurfaceView(Context context) {
        this(context, null);
    }

    public SurfaceView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public SurfaceView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        mRenderNode.requestPositionUpdates(this);

        setWillNotDraw(true);
    }

    /**
     * Return the SurfaceHolder providing access and control over this
     * SurfaceView's underlying surface.
     *
     * @return SurfaceHolder The holder of the surface.
     */
    public SurfaceHolder getHolder() {
        return mSurfaceHolder;
    }

    private void updateRequestedVisibility() {
        mRequestedVisible = mViewVisibility && mWindowVisibility && !mWindowStopped;
    }

    /** @hide */
    @Override
    public void windowStopped(boolean stopped) {
        mWindowStopped = stopped;
        updateRequestedVisibility();
        updateSurface();
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();

        getViewRootImpl().addWindowStoppedCallback(this);
        mWindowStopped = false;

        mViewVisibility = getVisibility() == VISIBLE;
        updateRequestedVisibility();

        mAttachedToWindow = true;
        mParent.requestTransparentRegion(SurfaceView.this);
        if (!mGlobalListenersAdded) {
            ViewTreeObserver observer = getViewTreeObserver();
            observer.addOnScrollChangedListener(mScrollChangedListener);
            observer.addOnPreDrawListener(mDrawListener);
            mGlobalListenersAdded = true;
        }
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        mWindowVisibility = visibility == VISIBLE;
        updateRequestedVisibility();
        updateSurface();
    }

    @Override
    public void setVisibility(int visibility) {
        super.setVisibility(visibility);
        mViewVisibility = visibility == VISIBLE;
        boolean newRequestedVisible = mWindowVisibility && mViewVisibility && !mWindowStopped;
        if (newRequestedVisible != mRequestedVisible) {
            // our base class (View) invalidates the layout only when
            // we go from/to the GONE state. However, SurfaceView needs
            // to request a re-layout when the visibility changes at all.
            // This is needed because the transparent region is computed
            // as part of the layout phase, and it changes (obviously) when
            // the visibility changes.
            requestLayout();
        }
        mRequestedVisible = newRequestedVisible;
        updateSurface();
    }

    private void performDrawFinished() {
        if (mPendingReportDraws > 0) {
            mDrawFinished = true;
            if (mAttachedToWindow) {
                notifyDrawFinished();
                invalidate();
            }
        } else {
            Log.e(TAG, System.identityHashCode(this) + "finished drawing"
                    + " but no pending report draw (extra call"
                    + " to draw completion runnable?)");
        }
    }

    void notifyDrawFinished() {
        ViewRootImpl viewRoot = getViewRootImpl();
        if (viewRoot != null) {
            viewRoot.pendingDrawFinished();
        }
        mPendingReportDraws--;
    }

    @Override
    protected void onDetachedFromWindow() {
        ViewRootImpl viewRoot = getViewRootImpl();
        // It's possible to create a SurfaceView using the default constructor and never
        // attach it to a view hierarchy, this is a common use case when dealing with
        // OpenGL. A developer will probably create a new GLSurfaceView, and let it manage
        // the lifecycle. Instead of attaching it to a view, he/she can just pass
        // the SurfaceHolder forward, most live wallpapers do it.
        if (viewRoot != null) {
            viewRoot.removeWindowStoppedCallback(this);
        }

        mAttachedToWindow = false;
        if (mGlobalListenersAdded) {
            ViewTreeObserver observer = getViewTreeObserver();
            observer.removeOnScrollChangedListener(mScrollChangedListener);
            observer.removeOnPreDrawListener(mDrawListener);
            mGlobalListenersAdded = false;
        }

        while (mPendingReportDraws > 0) {
            notifyDrawFinished();
        }

        mRequestedVisible = false;

        updateSurface();
        if (mSurfaceControl != null) {
            mSurfaceControl.destroy();
        }
        mSurfaceControl = null;

        mHaveFrame = false;

        super.onDetachedFromWindow();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = mRequestedWidth >= 0
                ? resolveSizeAndState(mRequestedWidth, widthMeasureSpec, 0)
                : getDefaultSize(0, widthMeasureSpec);
        int height = mRequestedHeight >= 0
                ? resolveSizeAndState(mRequestedHeight, heightMeasureSpec, 0)
                : getDefaultSize(0, heightMeasureSpec);
        setMeasuredDimension(width, height);
    }

    /** @hide */
    @Override
    protected boolean setFrame(int left, int top, int right, int bottom) {
        boolean result = super.setFrame(left, top, right, bottom);
        updateSurface();
        return result;
    }

    @Override
    public boolean gatherTransparentRegion(Region region) {
        if (isAboveParent() || !mDrawFinished) {
            return super.gatherTransparentRegion(region);
        }

        boolean opaque = true;
        if ((mPrivateFlags & PFLAG_SKIP_DRAW) == 0) {
            // this view draws, remove it from the transparent region
            opaque = super.gatherTransparentRegion(region);
        } else if (region != null) {
            int w = getWidth();
            int h = getHeight();
            if (w>0 && h>0) {
                getLocationInWindow(mLocation);
                // otherwise, punch a hole in the whole hierarchy
                int l = mLocation[0];
                int t = mLocation[1];
                region.op(l, t, l+w, t+h, Region.Op.UNION);
            }
        }
        if (PixelFormat.formatHasAlpha(mRequestedFormat)) {
            opaque = false;
        }
        return opaque;
    }

    @Override
    public void draw(Canvas canvas) {
        if (mDrawFinished && !isAboveParent()) {
            // draw() is not called when SKIP_DRAW is set
            if ((mPrivateFlags & PFLAG_SKIP_DRAW) == 0) {
                // punch a whole in the view-hierarchy below us
                canvas.drawColor(0, PorterDuff.Mode.CLEAR);
            }
        }
        super.draw(canvas);
    }

    @Override
    protected void dispatchDraw(Canvas canvas) {
        if (mDrawFinished && !isAboveParent()) {
            // draw() is not called when SKIP_DRAW is set
            if ((mPrivateFlags & PFLAG_SKIP_DRAW) == PFLAG_SKIP_DRAW) {
                // punch a whole in the view-hierarchy below us
                canvas.drawColor(0, PorterDuff.Mode.CLEAR);
            }
        }
        super.dispatchDraw(canvas);
    }

    /**
     * Control whether the surface view's surface is placed on top of another
     * regular surface view in the window (but still behind the window itself).
     * This is typically used to place overlays on top of an underlying media
     * surface view.
     *
     * <p>Note that this must be set before the surface view's containing
     * window is attached to the window manager.
     *
     * <p>Calling this overrides any previous call to {@link #setZOrderOnTop}.
     */
    public void setZOrderMediaOverlay(boolean isMediaOverlay) {
        mSubLayer = isMediaOverlay
            ? APPLICATION_MEDIA_OVERLAY_SUBLAYER : APPLICATION_MEDIA_SUBLAYER;
    }

    /**
     * Control whether the surface view's surface is placed on top of its
     * window.  Normally it is placed behind the window, to allow it to
     * (for the most part) appear to composite with the views in the
     * hierarchy.  By setting this, you cause it to be placed above the
     * window.  This means that none of the contents of the window this
     * SurfaceView is in will be visible on top of its surface.
     *
     * <p>Note that this must be set before the surface view's containing
     * window is attached to the window manager.
     *
     * <p>Calling this overrides any previous call to {@link #setZOrderMediaOverlay}.
     */
    public void setZOrderOnTop(boolean onTop) {
        if (onTop) {
            mSubLayer = APPLICATION_PANEL_SUBLAYER;
        } else {
            mSubLayer = APPLICATION_MEDIA_SUBLAYER;
        }
    }

    /**
     * Control whether the surface view's content should be treated as secure,
     * preventing it from appearing in screenshots or from being viewed on
     * non-secure displays.
     *
     * <p>Note that this must be set before the surface view's containing
     * window is attached to the window manager.
     *
     * <p>See {@link android.view.Display#FLAG_SECURE} for details.
     *
     * @param isSecure True if the surface view is secure.
     */
    public void setSecure(boolean isSecure) {
        if (isSecure) {
            mSurfaceFlags |= SurfaceControl.SECURE;
        } else {
            mSurfaceFlags &= ~SurfaceControl.SECURE;
        }
    }

    private void updateOpaqueFlag() {
        if (!PixelFormat.formatHasAlpha(mRequestedFormat)) {
            mSurfaceFlags |= SurfaceControl.OPAQUE;
        } else {
            mSurfaceFlags &= ~SurfaceControl.OPAQUE;
        }
    }

    private Rect getParentSurfaceInsets() {
        final ViewRootImpl root = getViewRootImpl();
        if (root == null) {
            return null;
        } else {
            return root.mWindowAttributes.surfaceInsets;
        }
    }

    /** @hide */
    protected void updateSurface() {
        if (!mHaveFrame) {
            return;
        }
        ViewRootImpl viewRoot = getViewRootImpl();
        if (viewRoot == null || viewRoot.mSurface == null || !viewRoot.mSurface.isValid()) {
            return;
        }

        mTranslator = viewRoot.mTranslator;
        if (mTranslator != null) {
            mSurface.setCompatibilityTranslator(mTranslator);
        }

        int myWidth = mRequestedWidth;
        if (myWidth <= 0) myWidth = getWidth();
        int myHeight = mRequestedHeight;
        if (myHeight <= 0) myHeight = getHeight();

        final boolean formatChanged = mFormat != mRequestedFormat;
        final boolean visibleChanged = mVisible != mRequestedVisible;
        final boolean creating = (mSurfaceControl == null || formatChanged || visibleChanged)
                && mRequestedVisible;
        final boolean sizeChanged = mSurfaceWidth != myWidth || mSurfaceHeight != myHeight;
        final boolean windowVisibleChanged = mWindowVisibility != mLastWindowVisibility;
        boolean redrawNeeded = false;

        if (creating || formatChanged || sizeChanged || visibleChanged || windowVisibleChanged) {
            getLocationInWindow(mLocation);

            if (DEBUG) Log.i(TAG, System.identityHashCode(this) + " "
                    + "Changes: creating=" + creating
                    + " format=" + formatChanged + " size=" + sizeChanged
                    + " visible=" + visibleChanged
                    + " left=" + (mWindowSpaceLeft != mLocation[0])
                    + " top=" + (mWindowSpaceTop != mLocation[1]));

            try {
                final boolean visible = mVisible = mRequestedVisible;
                mWindowSpaceLeft = mLocation[0];
                mWindowSpaceTop = mLocation[1];
                mSurfaceWidth = myWidth;
                mSurfaceHeight = myHeight;
                mFormat = mRequestedFormat;
                mLastWindowVisibility = mWindowVisibility;

                mScreenRect.left = mWindowSpaceLeft;
                mScreenRect.top = mWindowSpaceTop;
                mScreenRect.right = mWindowSpaceLeft + getWidth();
                mScreenRect.bottom = mWindowSpaceTop + getHeight();
                if (mTranslator != null) {
                    mTranslator.translateRectInAppWindowToScreen(mScreenRect);
                }

                final Rect surfaceInsets = getParentSurfaceInsets();
                mScreenRect.offset(surfaceInsets.left, surfaceInsets.top);

                if (creating) {
                    mSurfaceSession = new SurfaceSession(viewRoot.mSurface);
                    mDeferredDestroySurfaceControl = mSurfaceControl;

                    updateOpaqueFlag();
                    final String name = "SurfaceView - " + viewRoot.getTitle().toString();

                    mSurfaceControl = new SurfaceControlWithBackground(
                            name,
                            (mSurfaceFlags & SurfaceControl.OPAQUE) != 0,
                            new SurfaceControl.Builder(mSurfaceSession)
                                    .setSize(mSurfaceWidth, mSurfaceHeight)
                                    .setFormat(mFormat)
                                    .setFlags(mSurfaceFlags));
                } else if (mSurfaceControl == null) {
                    return;
                }

                boolean realSizeChanged = false;

                mSurfaceLock.lock();
                try {
                    mDrawingStopped = !visible;

                    if (DEBUG) Log.i(TAG, System.identityHashCode(this) + " "
                            + "Cur surface: " + mSurface);

                    SurfaceControl.openTransaction();
                    try {
                        mSurfaceControl.setLayer(mSubLayer);
                        if (mViewVisibility) {
                            mSurfaceControl.show();
                        } else {
                            mSurfaceControl.hide();
                        }

                        // While creating the surface, we will set it's initial
                        // geometry. Outside of that though, we should generally
                        // leave it to the RenderThread.
                        //
                        // There is one more case when the buffer size changes we aren't yet
                        // prepared to sync (as even following the transaction applying
                        // we still need to latch a buffer).
                        // b/28866173
                        if (sizeChanged || creating || !mRtHandlingPositionUpdates) {
                            mSurfaceControl.setPosition(mScreenRect.left, mScreenRect.top);
                            mSurfaceControl.setMatrix(mScreenRect.width() / (float) mSurfaceWidth,
                                    0.0f, 0.0f,
                                    mScreenRect.height() / (float) mSurfaceHeight);
                        }
                        if (sizeChanged) {
                            mSurfaceControl.setSize(mSurfaceWidth, mSurfaceHeight);
                        }
                    } finally {
                        SurfaceControl.closeTransaction();
                    }

                    if (sizeChanged || creating) {
                        redrawNeeded = true;
                    }

                    mSurfaceFrame.left = 0;
                    mSurfaceFrame.top = 0;
                    if (mTranslator == null) {
                        mSurfaceFrame.right = mSurfaceWidth;
                        mSurfaceFrame.bottom = mSurfaceHeight;
                    } else {
                        float appInvertedScale = mTranslator.applicationInvertedScale;
                        mSurfaceFrame.right = (int) (mSurfaceWidth * appInvertedScale + 0.5f);
                        mSurfaceFrame.bottom = (int) (mSurfaceHeight * appInvertedScale + 0.5f);
                    }

                    final int surfaceWidth = mSurfaceFrame.right;
                    final int surfaceHeight = mSurfaceFrame.bottom;
                    realSizeChanged = mLastSurfaceWidth != surfaceWidth
                            || mLastSurfaceHeight != surfaceHeight;
                    mLastSurfaceWidth = surfaceWidth;
                    mLastSurfaceHeight = surfaceHeight;
                } finally {
                    mSurfaceLock.unlock();
                }

                try {
                    redrawNeeded |= visible && !mDrawFinished;

                    SurfaceHolder.Callback callbacks[] = null;

                    final boolean surfaceChanged = creating;
                    if (mSurfaceCreated && (surfaceChanged || (!visible && visibleChanged))) {
                        mSurfaceCreated = false;
                        if (mSurface.isValid()) {
                            if (DEBUG) Log.i(TAG, System.identityHashCode(this) + " "
                                    + "visibleChanged -- surfaceDestroyed");
                            callbacks = getSurfaceCallbacks();
                            for (SurfaceHolder.Callback c : callbacks) {
                                c.surfaceDestroyed(mSurfaceHolder);
                            }
                            // Since Android N the same surface may be reused and given to us
                            // again by the system server at a later point. However
                            // as we didn't do this in previous releases, clients weren't
                            // necessarily required to clean up properly in
                            // surfaceDestroyed. This leads to problems for example when
                            // clients don't destroy their EGL context, and try
                            // and create a new one on the same surface following reuse.
                            // Since there is no valid use of the surface in-between
                            // surfaceDestroyed and surfaceCreated, we force a disconnect,
                            // so the next connect will always work if we end up reusing
                            // the surface.
                            if (mSurface.isValid()) {
                                mSurface.forceScopedDisconnect();
                            }
                        }
                    }

                    if (creating) {
                        mSurface.copyFrom(mSurfaceControl);
                    }

                    if (sizeChanged && getContext().getApplicationInfo().targetSdkVersion
                            < Build.VERSION_CODES.O) {
                        // Some legacy applications use the underlying native {@link Surface} object
                        // as a key to whether anything has changed. In these cases, updates to the
                        // existing {@link Surface} will be ignored when the size changes.
                        // Therefore, we must explicitly recreate the {@link Surface} in these
                        // cases.
                        mSurface.createFrom(mSurfaceControl);
                    }

                    if (visible && mSurface.isValid()) {
                        if (!mSurfaceCreated && (surfaceChanged || visibleChanged)) {
                            mSurfaceCreated = true;
                            mIsCreating = true;
                            if (DEBUG) Log.i(TAG, System.identityHashCode(this) + " "
                                    + "visibleChanged -- surfaceCreated");
                            if (callbacks == null) {
                                callbacks = getSurfaceCallbacks();
                            }
                            for (SurfaceHolder.Callback c : callbacks) {
                                c.surfaceCreated(mSurfaceHolder);
                            }
                        }
                        if (creating || formatChanged || sizeChanged
                                || visibleChanged || realSizeChanged) {
                            if (DEBUG) Log.i(TAG, System.identityHashCode(this) + " "
                                    + "surfaceChanged -- format=" + mFormat
                                    + " w=" + myWidth + " h=" + myHeight);
                            if (callbacks == null) {
                                callbacks = getSurfaceCallbacks();
                            }
                            for (SurfaceHolder.Callback c : callbacks) {
                                c.surfaceChanged(mSurfaceHolder, mFormat, myWidth, myHeight);
                            }
                        }
                        if (redrawNeeded) {
                            if (DEBUG) Log.i(TAG, System.identityHashCode(this) + " "
                                    + "surfaceRedrawNeeded");
                            if (callbacks == null) {
                                callbacks = getSurfaceCallbacks();
                            }

                            mPendingReportDraws++;
                            viewRoot.drawPending();
                            SurfaceCallbackHelper sch =
                                    new SurfaceCallbackHelper(this::onDrawFinished);
                            sch.dispatchSurfaceRedrawNeededAsync(mSurfaceHolder, callbacks);
                        }
                    }
                } finally {
                    mIsCreating = false;
                    if (mSurfaceControl != null && !mSurfaceCreated) {
                        mSurface.release();

                        mSurfaceControl.destroy();
                        mSurfaceControl = null;
                    }
                }
            } catch (Exception ex) {
                Log.e(TAG, "Exception configuring surface", ex);
            }
            if (DEBUG) Log.v(
                TAG, "Layout: x=" + mScreenRect.left + " y=" + mScreenRect.top
                + " w=" + mScreenRect.width() + " h=" + mScreenRect.height()
                + ", frame=" + mSurfaceFrame);
        } else {
            // Calculate the window position in case RT loses the window
            // and we need to fallback to a UI-thread driven position update
            getLocationInSurface(mLocation);
            final boolean positionChanged = mWindowSpaceLeft != mLocation[0]
                    || mWindowSpaceTop != mLocation[1];
            final boolean layoutSizeChanged = getWidth() != mScreenRect.width()
                    || getHeight() != mScreenRect.height();
            if (positionChanged || layoutSizeChanged) { // Only the position has changed
                mWindowSpaceLeft = mLocation[0];
                mWindowSpaceTop = mLocation[1];
                // For our size changed check, we keep mScreenRect.width() and mScreenRect.height()
                // in view local space.
                mLocation[0] = getWidth();
                mLocation[1] = getHeight();

                mScreenRect.set(mWindowSpaceLeft, mWindowSpaceTop,
                        mWindowSpaceLeft + mLocation[0], mWindowSpaceTop + mLocation[1]);

                if (mTranslator != null) {
                    mTranslator.translateRectInAppWindowToScreen(mScreenRect);
                }

                if (mSurfaceControl == null) {
                    return;
                }

                if (!isHardwareAccelerated() || !mRtHandlingPositionUpdates) {
                    try {
                        if (DEBUG) Log.d(TAG, String.format("%d updateSurfacePosition UI, " +
                                "postion = [%d, %d, %d, %d]", System.identityHashCode(this),
                                mScreenRect.left, mScreenRect.top,
                                mScreenRect.right, mScreenRect.bottom));
                        setParentSpaceRectangle(mScreenRect, -1);
                    } catch (Exception ex) {
                        Log.e(TAG, "Exception configuring surface", ex);
                    }
                }
            }
        }
    }

    private void onDrawFinished() {
        if (DEBUG) {
            Log.i(TAG, System.identityHashCode(this) + " "
                    + "finishedDrawing");
        }

        if (mDeferredDestroySurfaceControl != null) {
            mDeferredDestroySurfaceControl.destroy();
            mDeferredDestroySurfaceControl = null;
        }

        runOnUiThread(() -> {
            performDrawFinished();
        });
    }

    /**
     * A place to over-ride for applying child-surface transactions.
     * These can be synchronized with the viewroot surface using deferTransaction.
     *
     * Called from RenderWorker while UI thread is paused.
     * @hide
     */
    protected void applyChildSurfaceTransaction_renderWorker(SurfaceControl.Transaction t,
            Surface viewRootSurface, long nextViewRootFrameNumber) {
    }

    private void applySurfaceTransforms(SurfaceControl surface, Rect position, long frameNumber) {
        if (frameNumber > 0) {
            final ViewRootImpl viewRoot = getViewRootImpl();

            mRtTransaction.deferTransactionUntilSurface(surface, viewRoot.mSurface,
                    frameNumber);
        }

        mRtTransaction.setPosition(surface, position.left, position.top);
        mRtTransaction.setMatrix(surface,
                position.width() / (float) mSurfaceWidth,
                0.0f, 0.0f,
                position.height() / (float) mSurfaceHeight);
    }

    private void setParentSpaceRectangle(Rect position, long frameNumber) {
        final ViewRootImpl viewRoot = getViewRootImpl();

        applySurfaceTransforms(mSurfaceControl, position, frameNumber);
        applySurfaceTransforms(mSurfaceControl.mBackgroundControl, position, frameNumber);

        applyChildSurfaceTransaction_renderWorker(mRtTransaction, viewRoot.mSurface,
                frameNumber);

        mRtTransaction.apply();
    }

    private Rect mRTLastReportedPosition = new Rect();

    /**
     * Called by native by a Rendering Worker thread to update the window position
     * @hide
     */
    public final void updateSurfacePosition_renderWorker(long frameNumber,
            int left, int top, int right, int bottom) {
        if (mSurfaceControl == null) {
            return;
        }

        // TODO: This is teensy bit racey in that a brand new SurfaceView moving on
        // its 2nd frame if RenderThread is running slowly could potentially see
        // this as false, enter the branch, get pre-empted, then this comes along
        // and reports a new position, then the UI thread resumes and reports
        // its position. This could therefore be de-sync'd in that interval, but
        // the synchronization would violate the rule that RT must never block
        // on the UI thread which would open up potential deadlocks. The risk of
        // a single-frame desync is therefore preferable for now.
        mRtHandlingPositionUpdates = true;
        if (mRTLastReportedPosition.left == left
                && mRTLastReportedPosition.top == top
                && mRTLastReportedPosition.right == right
                && mRTLastReportedPosition.bottom == bottom) {
            return;
        }
        try {
            if (DEBUG) {
                Log.d(TAG, String.format("%d updateSurfacePosition RenderWorker, frameNr = %d, " +
                        "postion = [%d, %d, %d, %d]", System.identityHashCode(this),
                        frameNumber, left, top, right, bottom));
            }
            mRTLastReportedPosition.set(left, top, right, bottom);
            setParentSpaceRectangle(mRTLastReportedPosition, frameNumber);
            // Now overwrite mRTLastReportedPosition with our values
        } catch (Exception ex) {
            Log.e(TAG, "Exception from repositionChild", ex);
        }
    }

    /**
     * Called by native on RenderThread to notify that the view is no longer in the
     * draw tree. UI thread is blocked at this point.
     * @hide
     */
    public final void surfacePositionLost_uiRtSync(long frameNumber) {
        if (DEBUG) {
            Log.d(TAG, String.format("%d windowPositionLost, frameNr = %d",
                    System.identityHashCode(this), frameNumber));
        }
        mRTLastReportedPosition.setEmpty();

        if (mSurfaceControl == null) {
            return;
        }
        if (mRtHandlingPositionUpdates) {
            mRtHandlingPositionUpdates = false;
            // This callback will happen while the UI thread is blocked, so we can
            // safely access other member variables at this time.
            // So do what the UI thread would have done if RT wasn't handling position
            // updates.
            if (!mScreenRect.isEmpty() && !mScreenRect.equals(mRTLastReportedPosition)) {
                try {
                    if (DEBUG) Log.d(TAG, String.format("%d updateSurfacePosition, " +
                            "postion = [%d, %d, %d, %d]", System.identityHashCode(this),
                            mScreenRect.left, mScreenRect.top,
                            mScreenRect.right, mScreenRect.bottom));
                    setParentSpaceRectangle(mScreenRect, frameNumber);
                } catch (Exception ex) {
                    Log.e(TAG, "Exception configuring surface", ex);
                }
            }
        }
    }

    private SurfaceHolder.Callback[] getSurfaceCallbacks() {
        SurfaceHolder.Callback callbacks[];
        synchronized (mCallbacks) {
            callbacks = new SurfaceHolder.Callback[mCallbacks.size()];
            mCallbacks.toArray(callbacks);
        }
        return callbacks;
    }

    private void runOnUiThread(Runnable runnable) {
        Handler handler = getHandler();
        if (handler != null && handler.getLooper() != Looper.myLooper()) {
            handler.post(runnable);
        } else {
            runnable.run();
        }
    }

    /**
     * Check to see if the surface has fixed size dimensions or if the surface's
     * dimensions are dimensions are dependent on its current layout.
     *
     * @return true if the surface has dimensions that are fixed in size
     * @hide
     */
    public boolean isFixedSize() {
        return (mRequestedWidth != -1 || mRequestedHeight != -1);
    }

    private boolean isAboveParent() {
        return mSubLayer >= 0;
    }

    /**
     * Set an opaque background color to use with this {@link SurfaceView} when it's being resized
     * and size of the content hasn't updated yet. This color will fill the expanded area when the
     * view becomes larger.
     * @param bgColor An opaque color to fill the background. Alpha component will be ignored.
     * @hide
     */
    public void setResizeBackgroundColor(int bgColor) {
        mSurfaceControl.setBackgroundColor(bgColor);
    }

    private final SurfaceHolder mSurfaceHolder = new SurfaceHolder() {
        private static final String LOG_TAG = "SurfaceHolder";

        @Override
        public boolean isCreating() {
            return mIsCreating;
        }

        @Override
        public void addCallback(Callback callback) {
            synchronized (mCallbacks) {
                // This is a linear search, but in practice we'll
                // have only a couple callbacks, so it doesn't matter.
                if (mCallbacks.contains(callback) == false) {
                    mCallbacks.add(callback);
                }
            }
        }

        @Override
        public void removeCallback(Callback callback) {
            synchronized (mCallbacks) {
                mCallbacks.remove(callback);
            }
        }

        @Override
        public void setFixedSize(int width, int height) {
            if (mRequestedWidth != width || mRequestedHeight != height) {
                mRequestedWidth = width;
                mRequestedHeight = height;
                requestLayout();
            }
        }

        @Override
        public void setSizeFromLayout() {
            if (mRequestedWidth != -1 || mRequestedHeight != -1) {
                mRequestedWidth = mRequestedHeight = -1;
                requestLayout();
            }
        }

        @Override
        public void setFormat(int format) {
            // for backward compatibility reason, OPAQUE always
            // means 565 for SurfaceView
            if (format == PixelFormat.OPAQUE)
                format = PixelFormat.RGB_565;

            mRequestedFormat = format;
            if (mSurfaceControl != null) {
                updateSurface();
            }
        }

        /**
         * @deprecated setType is now ignored.
         */
        @Override
        @Deprecated
        public void setType(int type) { }

        @Override
        public void setKeepScreenOn(boolean screenOn) {
            runOnUiThread(() -> SurfaceView.this.setKeepScreenOn(screenOn));
        }

        /**
         * Gets a {@link Canvas} for drawing into the SurfaceView's Surface
         *
         * After drawing into the provided {@link Canvas}, the caller must
         * invoke {@link #unlockCanvasAndPost} to post the new contents to the surface.
         *
         * The caller must redraw the entire surface.
         * @return A canvas for drawing into the surface.
         */
        @Override
        public Canvas lockCanvas() {
            return internalLockCanvas(null, false);
        }

        /**
         * Gets a {@link Canvas} for drawing into the SurfaceView's Surface
         *
         * After drawing into the provided {@link Canvas}, the caller must
         * invoke {@link #unlockCanvasAndPost} to post the new contents to the surface.
         *
         * @param inOutDirty A rectangle that represents the dirty region that the caller wants
         * to redraw.  This function may choose to expand the dirty rectangle if for example
         * the surface has been resized or if the previous contents of the surface were
         * not available.  The caller must redraw the entire dirty region as represented
         * by the contents of the inOutDirty rectangle upon return from this function.
         * The caller may also pass <code>null</code> instead, in the case where the
         * entire surface should be redrawn.
         * @return A canvas for drawing into the surface.
         */
        @Override
        public Canvas lockCanvas(Rect inOutDirty) {
            return internalLockCanvas(inOutDirty, false);
        }

        @Override
        public Canvas lockHardwareCanvas() {
            return internalLockCanvas(null, true);
        }

        private Canvas internalLockCanvas(Rect dirty, boolean hardware) {
            mSurfaceLock.lock();

            if (DEBUG) Log.i(TAG, System.identityHashCode(this) + " " + "Locking canvas... stopped="
                    + mDrawingStopped + ", surfaceControl=" + mSurfaceControl);

            Canvas c = null;
            if (!mDrawingStopped && mSurfaceControl != null) {
                try {
                    if (hardware) {
                        c = mSurface.lockHardwareCanvas();
                    } else {
                        c = mSurface.lockCanvas(dirty);
                    }
                } catch (Exception e) {
                    Log.e(LOG_TAG, "Exception locking surface", e);
                }
            }

            if (DEBUG) Log.i(TAG, System.identityHashCode(this) + " " + "Returned canvas: " + c);
            if (c != null) {
                mLastLockTime = SystemClock.uptimeMillis();
                return c;
            }

            // If the Surface is not ready to be drawn, then return null,
            // but throttle calls to this function so it isn't called more
            // than every 100ms.
            long now = SystemClock.uptimeMillis();
            long nextTime = mLastLockTime + 100;
            if (nextTime > now) {
                try {
                    Thread.sleep(nextTime-now);
                } catch (InterruptedException e) {
                }
                now = SystemClock.uptimeMillis();
            }
            mLastLockTime = now;
            mSurfaceLock.unlock();

            return null;
        }

        /**
         * Posts the new contents of the {@link Canvas} to the surface and
         * releases the {@link Canvas}.
         *
         * @param canvas The canvas previously obtained from {@link #lockCanvas}.
         */
        @Override
        public void unlockCanvasAndPost(Canvas canvas) {
            mSurface.unlockCanvasAndPost(canvas);
            mSurfaceLock.unlock();
        }

        @Override
        public Surface getSurface() {
            return mSurface;
        }

        @Override
        public Rect getSurfaceFrame() {
            return mSurfaceFrame;
        }
    };

    class SurfaceControlWithBackground extends SurfaceControl {
        SurfaceControl mBackgroundControl;
        private boolean mOpaque = true;
        public boolean mVisible = false;

        public SurfaceControlWithBackground(String name, boolean opaque, SurfaceControl.Builder b)
                       throws Exception {
            super(b.setName(name).build());

            mBackgroundControl = b.setName("Background for -" + name)
                    .setFormat(OPAQUE)
                    .setColorLayer(true)
                    .build();
            mOpaque = opaque;
        }

        @Override
        public void setAlpha(float alpha) {
            super.setAlpha(alpha);
            mBackgroundControl.setAlpha(alpha);
        }

        @Override
        public void setLayer(int zorder) {
            super.setLayer(zorder);
            // -3 is below all other child layers as SurfaceView never goes below -2
            mBackgroundControl.setLayer(-3);
        }

        @Override
        public void setPosition(float x, float y) {
            super.setPosition(x, y);
            mBackgroundControl.setPosition(x, y);
        }

        @Override
        public void setSize(int w, int h) {
            super.setSize(w, h);
            mBackgroundControl.setSize(w, h);
        }

        @Override
        public void setWindowCrop(Rect crop) {
            super.setWindowCrop(crop);
            mBackgroundControl.setWindowCrop(crop);
        }

        @Override
        public void setFinalCrop(Rect crop) {
            super.setFinalCrop(crop);
            mBackgroundControl.setFinalCrop(crop);
        }

        @Override
        public void setLayerStack(int layerStack) {
            super.setLayerStack(layerStack);
            mBackgroundControl.setLayerStack(layerStack);
        }

        @Override
        public void setOpaque(boolean isOpaque) {
            super.setOpaque(isOpaque);
            mOpaque = isOpaque;
            updateBackgroundVisibility();
        }

        @Override
        public void setSecure(boolean isSecure) {
            super.setSecure(isSecure);
        }

        @Override
        public void setMatrix(float dsdx, float dtdx, float dsdy, float dtdy) {
            super.setMatrix(dsdx, dtdx, dsdy, dtdy);
            mBackgroundControl.setMatrix(dsdx, dtdx, dsdy, dtdy);
        }

        @Override
        public void hide() {
            super.hide();
            mVisible = false;
            updateBackgroundVisibility();
        }

        @Override
        public void show() {
            super.show();
            mVisible = true;
            updateBackgroundVisibility();
        }

        @Override
        public void destroy() {
            super.destroy();
            mBackgroundControl.destroy();
         }

        @Override
        public void release() {
            super.release();
            mBackgroundControl.release();
        }

        @Override
        public void setTransparentRegionHint(Region region) {
            super.setTransparentRegionHint(region);
            mBackgroundControl.setTransparentRegionHint(region);
        }

        @Override
        public void deferTransactionUntil(IBinder handle, long frame) {
            super.deferTransactionUntil(handle, frame);
            mBackgroundControl.deferTransactionUntil(handle, frame);
        }

        @Override
        public void deferTransactionUntil(Surface barrier, long frame) {
            super.deferTransactionUntil(barrier, frame);
            mBackgroundControl.deferTransactionUntil(barrier, frame);
        }

        /** Set the color to fill the background with. */
        private void setBackgroundColor(int bgColor) {
            final float[] colorComponents = new float[] { Color.red(bgColor) / 255.f,
                    Color.green(bgColor) / 255.f, Color.blue(bgColor) / 255.f };

            SurfaceControl.openTransaction();
            try {
                mBackgroundControl.setColor(colorComponents);
            } finally {
                SurfaceControl.closeTransaction();
            }
        }

        void updateBackgroundVisibility() {
            if (mOpaque && mVisible) {
                mBackgroundControl.show();
            } else {
                mBackgroundControl.hide();
            }
        }
    }
}
