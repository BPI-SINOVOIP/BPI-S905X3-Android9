package com.android.tv.ui.hideable;

import android.content.Context;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.annotation.UiThread;
import android.support.annotation.VisibleForTesting;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityManager.AccessibilityStateChangeListener;
import com.android.tv.common.WeakHandler;

/**
 * Schedules a view element to be hidden after a delay.
 *
 * <p>When accessibility is turned on elements are not automatically hidden.
 *
 * <p>Users of this class must pass it to {@link
 * AccessibilityManager#addAccessibilityStateChangeListener(AccessibilityStateChangeListener)} and
 * {@link
 * AccessibilityManager#removeAccessibilityStateChangeListener(AccessibilityStateChangeListener)}
 * during the appropriate live cycle event, or handle calling {@link
 * #onAccessibilityStateChanged(boolean)}.
 */
@UiThread
public final class AutoHideScheduler implements AccessibilityStateChangeListener {
    private static final int MSG_HIDE = 1;

    private final HideHandler mHandler;
    private final Runnable mRunnable;

    public AutoHideScheduler(Context context, Runnable runnable) {
        this(
                runnable,
                context.getSystemService(AccessibilityManager.class),
                Looper.getMainLooper());
    }

    @VisibleForTesting
    AutoHideScheduler(Runnable runnable, AccessibilityManager accessibilityManager, Looper looper) {
        // Keep a reference here because HideHandler only has a weak reference to it.
        mRunnable = runnable;
        mHandler = new HideHandler(looper, mRunnable);
        mHandler.setAllowAutoHide(!accessibilityManager.isEnabled());
    }

    public void cancel() {
        mHandler.removeMessages(MSG_HIDE);
    }

    public void schedule(long delayMs) {
        cancel();
        if (mHandler.mAllowAutoHide) {
            mHandler.sendEmptyMessageDelayed(MSG_HIDE, delayMs);
        }
    }

    @Override
    public void onAccessibilityStateChanged(boolean enabled) {
        mHandler.onAccessibilityStateChanged(enabled);
    }

    public boolean isScheduled() {
        return mHandler.hasMessages(MSG_HIDE);
    }

    private static class HideHandler extends WeakHandler<Runnable>
            implements AccessibilityStateChangeListener {

        private boolean mAllowAutoHide;

        public HideHandler(Looper looper, Runnable hideRunner) {
            super(looper, hideRunner);
        }

        @Override
        protected void handleMessage(Message msg, @NonNull Runnable runnable) {
            switch (msg.what) {
                case MSG_HIDE:
                    if (mAllowAutoHide) {
                        runnable.run();
                    }
                    break;
                default:
                    // do nothing
            }
        }

        public void setAllowAutoHide(boolean mAllowAutoHide) {
            this.mAllowAutoHide = mAllowAutoHide;
        }

        @Override
        public void onAccessibilityStateChanged(boolean enabled) {
            mAllowAutoHide = !enabled;
        }
    }
}
